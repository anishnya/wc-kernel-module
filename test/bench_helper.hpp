#ifndef WC_BENCH_HELPER_HPP
#define WC_BENCH_HPP

#include <immintrin.h>

#include <cstdint>
#include <cstring>  // Add at top of file for memcpy

#include "constants.h"

// Cache operations
void flush_cache(void *ptr, const std::size_t size, const bool write_back)
{
    char *p = static_cast<char *>(ptr);
    char *end = p + size;

    for (; p < end; p += CACHE_LINE_SIZE)
    {
        _mm_clflushopt(p);
    }

    if (write_back)
    {
        _mm_sfence();
    }

    return;
}

/*
    Read functions
*/

void read_buffer(void *ptr, const std::size_t size, const std::uint64_t expected_pattern)
{
    const volatile std::uint64_t *buf = static_cast<const volatile std::uint64_t *>(ptr);
    const std::size_t num_elements = size / sizeof(std::uint64_t);

    for (std::size_t i = 0; i < num_elements; i++)
    {
        if (buf[i] != expected_pattern)
        {
            DEBUG_PRINT("Verification failed at offset " << i << ": expected 0x" << std::hex
                                                         << expected_pattern << ", got 0x" << buf[i]
                                                         << std::dec);
            std::exit(1);
        }
    }
}

void read_buffer_nt(void *ptr, const std::size_t size, const std::uint64_t expected_pattern)
{
    const char *buf = static_cast<const char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Create vector with expected pattern replicated
    const __m512i expected = _mm512_set1_epi64(expected_pattern);

    // Check 64-byte chunks using NT loads
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        __m512i loaded = _mm512_stream_load_si512((__m512i *)(buf + i * vec_size));
        __mmask8 comparison = _mm512_cmpeq_epi64_mask(loaded, expected);

        if (comparison != ALL_EQUAL_MASK)
        {
            DEBUG_PRINT("[DEBUG] Vector verification failed at offset %zu\n", i * vec_size);
            std::exit(1);
        }
    }

    return;
}

void read_buffer_avx512(void *ptr, const std::size_t size, const std::uint64_t expected_pattern)
{
    const char *buf = static_cast<const char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Create vector with expected pattern replicated
    const __m512i expected = _mm512_set1_epi64(expected_pattern);

    // Check 64-byte chunks using regular AVX-512 loads
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        __m512i loaded = _mm512_load_si512((__m512i *)(buf + i * vec_size));
        __mmask8 comparison = _mm512_cmpeq_epi64_mask(loaded, expected);

        if (comparison != ALL_EQUAL_MASK)
        {
            DEBUG_PRINT("[DEBUG] Vector verification failed at offset " << i * vec_size);
            std::exit(1);
        }
    }

    return;
}

/*
    Write functions
*/

void write_buffer(void *ptr, const std::size_t size, const std::uint64_t pattern)
{
    volatile std::uint64_t *buf = static_cast<volatile std::uint64_t *>(ptr);
    const std::size_t num_elements = size / sizeof(std::uint64_t);

    for (std::size_t i = 0; i < num_elements; i++)
    {
        buf[i] = pattern;
    }

    return;
}

void write_buffer_nt(void *ptr, const std::size_t size, const std::uint64_t pattern)
{
    char *buf = static_cast<char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Create vector with pattern replicated
    const __m512i pattern_vec = _mm512_set1_epi64(pattern);

    // Write 64-byte chunks using NT stores
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        _mm512_stream_si512((__m512i *)(buf + i * vec_size), pattern_vec);
    }
}

void write_buffer_avx512(void *ptr, const std::size_t size, const std::uint64_t pattern)
{
    char *buf = static_cast<char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Create vector with pattern replicated
    const __m512i pattern_vec = _mm512_set1_epi64(pattern);

    // Write 64-byte chunks using regular AVX-512 stores
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        _mm512_store_si512((__m512i *)(buf + i * vec_size), pattern_vec);
    }

    return;
}

/*
    Copy functions
*/

void copy_buffer_fast(void *dst, const void *src, const std::size_t size)
{
    std::memcpy(dst, src, size);
}

void copy_buffer_sequential(void *dst, const void *src, const std::size_t size)
{
    const std::uint64_t *src_ptr = static_cast<const std::uint64_t *>(src);
    std::uint64_t *dst_ptr = static_cast<std::uint64_t *>(dst);
    const std::size_t num_elements = size / sizeof(std::uint64_t);

    for (std::size_t i = 0; i < num_elements; i++)
    {
        dst_ptr[i] = src_ptr[i];
    }

    return;
}

// Copy from a non-temporal source to a temporal destination
void copy_buffer_nt_src(void *dst, const void *src, const std::size_t size)
{
    const char *src_ptr = static_cast<const char *>(src);
    char *dst_ptr = static_cast<char *>(dst);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Copy 64-byte chunks using NT loads and regular stores
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        __m512i loaded = _mm512_stream_load_si512((__m512i *)(src_ptr + i * vec_size));
        _mm512_store_si512((__m512i *)(dst_ptr + i * vec_size), loaded);
    }

    return;
}

// Copy from a temporal source to a non-temporal destination
void copy_buffer_nt_dst(void *dst, const void *src, const std::size_t size)
{
    const char *src_ptr = static_cast<const char *>(src);
    char *dst_ptr = static_cast<char *>(dst);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Copy 64-byte chunks using regular loads and NT stores
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        __m512i loaded = _mm512_load_si512((__m512i *)(src_ptr + i * vec_size));
        _mm512_stream_si512((__m512i *)(dst_ptr + i * vec_size), loaded);
    }
}

// Copy using non-temporal loads and stores
void copy_buffer_nt_both(void *dst, const void *src, const std::size_t size)
{
    const char *src_ptr = static_cast<const char *>(src);
    char *dst_ptr = static_cast<char *>(dst);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vectors = size / vec_size;

    // Copy 64-byte chunks using NT loads and NT stores
    for (std::size_t i = 0; i < num_vectors; i++)
    {
        __m512i loaded = _mm512_stream_load_si512((__m512i *)(src_ptr + i * vec_size));
        _mm512_stream_si512((__m512i *)(dst_ptr + i * vec_size), loaded);
    }

    return;
}

#endif  // WC_BENCH_HELPER_HPP