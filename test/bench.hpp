#ifndef WC_BENCH_HPP
#define WC_BENCH_HPP

#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include "stats.hpp"
#include "constants.h"
#include <immintrin.h> // Add this at the top with other includes
#include <cstring>     // Add to includes section

// Debug macro and helper
#ifdef WC_DEBUG
#define DEBUG_PRINT(...)     \
    do                       \
    {                        \
        printf(__VA_ARGS__); \
    } while (0)
#else
#define DEBUG_PRINT(...) \
    do                   \
    {                    \
    } while (0)
#endif

void print_benchmark_results(bool uncached_mem_test, size_t size, size_t reads,
                             uint64_t elapsed_ns, uint64_t sum)
{
    double seconds = elapsed_ns / 1e9;
    double bandwidth = (size / (1024.0 * 1024.0)) / seconds; // MB/s

    if (!(sum ^ (reads << 2)) && sum == 0)
    {
        std::cout << "Error: Sum verification failed!" << std::endl;
        std::cout << (uncached_mem_test ? "uncached" : "cached")
                  << " mem test: buffer size: " << size
                  << " bytes, " << reads << " reads in "
                  << std::fixed << std::setprecision(2) << seconds
                  << "s (bandwidth: " << std::setprecision(2) << bandwidth
                  << " MB/s, sum: " << sum << ")" << std::endl;
        exit(1);
    }
}

void run_copy_benchmark(void *src, void *dst, size_t size, HistogramStats &copy_stats)
{
    // Time copy operation
    auto copy_start = std::chrono::high_resolution_clock::now();
    std::memcpy(dst, src, size);
    uint64_t copy_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now() - copy_start)
                           .count();
    copy_stats.event(copy_ns);

    DEBUG_PRINT("[DEBUG] Copied %zu bytes in %lu ns\n", size, copy_ns);
}

void run_copy_benchmark_nt(void *src, void *dst, size_t size, HistogramStats &copy_stats)
{
    // Time copy operation
    auto copy_start = std::chrono::high_resolution_clock::now();

    // Process 512 bits (64 bytes) at a time using AVX-512 pointers
    __m512i *src_vec = reinterpret_cast<__m512i *>(src);
    __m512i *dst_vec = reinterpret_cast<__m512i *>(dst);
    size_t vec_count = size / sizeof(__m512i);

    for (size_t i = 0; i < vec_count; i++)
    {
        __m512i v = _mm512_stream_load_si512(&src_vec[i]);
        _mm512_store_si512(&dst_vec[i], v);
    }

    uint64_t copy_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::high_resolution_clock::now() - copy_start)
                           .count();
    copy_stats.event(copy_ns);

    DEBUG_PRINT("[DEBUG] NT-load copied %zu bytes in %lu ns\n", size, copy_ns);
}

void run_read_benchmark(void *map, size_t size, bool uncached_mem_test, HistogramStats &stats)
{
    DEBUG_PRINT("[DEBUG] Starting regular read benchmark: size=%zu bytes\n", size);
    auto time_start = std::chrono::high_resolution_clock::now();

    volatile uint64_t *pt = ((volatile uint64_t *)map);
    size_t tsize = size / sizeof(uint64_t);
    uint64_t sum = 0;
    size_t reads = 0;

    DEBUG_PRINT("[DEBUG] Reading %zu uint64_t elements\n", tsize);
    for (size_t i = 0; i < tsize; i++)
    {
        sum += pt[i];
        reads++;
    }
    DEBUG_PRINT("[DEBUG] Completed %zu reads, sum=0x%lx\n", reads, sum);

    uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now() - time_start)
                              .count();
    DEBUG_PRINT("[DEBUG] Elapsed time: %lu ns\n", elapsed_ns);

    stats.event(elapsed_ns);
    print_benchmark_results(uncached_mem_test, size, reads, elapsed_ns, sum);
}

void run_read_benchmark_nt(void *map, size_t size, bool uncached_mem_test, HistogramStats &stats)
{
    DEBUG_PRINT("[DEBUG] Starting non-temporal read benchmark: size=%zu bytes\n", size);
    auto time_start = std::chrono::high_resolution_clock::now();

    volatile uint64_t *pt = static_cast<volatile uint64_t *>(map);
    size_t tsize = (size / sizeof(uint64_t));
    size_t reads = 0;

    uint64_t tmp[8];
    __m512i v;
    __m512i sum_vec = _mm512_setzero_si512(); // Initialize sum vector to zero
    DEBUG_PRINT("[DEBUG] Reading %zu uint64_t elements with NT loads\n", tsize);

    for (size_t i = 0; i < tsize; i += 8)
    {
        v = _mm512_stream_load_si512((__m512i *)(pt + i));
        sum_vec = _mm512_add_epi64(sum_vec, v); // Add vectors
        reads += 8;
    }

    // Reduce vector sum to scalar
    _mm512_store_si512((__m512i *)tmp, sum_vec);
    uint64_t sum = 0;
    for (int j = 0; j < 8; j++)
    {
        sum += tmp[j];
    }

    DEBUG_PRINT("[DEBUG] Completed %zu NT reads, sum=0x%lx\n", reads, sum);

    uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now() - time_start)
                              .count();

    stats.event(elapsed_ns);
    print_benchmark_results(uncached_mem_test, size, reads, elapsed_ns, sum);
}

void run_write_benchmark(void *map, size_t size, bool uncached_mem_test, HistogramStats &stats)
{
    DEBUG_PRINT("[DEBUG] Starting regular write benchmark: size=%zu bytes\n", size);
    auto time_start = std::chrono::high_resolution_clock::now();

    volatile uint64_t *pt = ((volatile uint64_t *)map);
    size_t tsize = size / sizeof(uint64_t);
    uint64_t value = 0xDEADBEEF; // Test pattern
    size_t writes = 0;

    DEBUG_PRINT("[DEBUG] Writing %zu uint64_t elements with value 0x%lx\n", tsize, value);
    // Simple sequential write through the buffer
    for (size_t i = 0; i < tsize; i++)
    {
        pt[i] = value;
        writes++;
    }
    DEBUG_PRINT("[DEBUG] Completed %zu writes\n", writes);

    uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now() - time_start)
                              .count();
    DEBUG_PRINT("[DEBUG] Elapsed time: %lu ns\n", elapsed_ns);

    stats.event(elapsed_ns);
    print_benchmark_results(uncached_mem_test, size, writes, elapsed_ns, value);
}

void run_write_benchmark_nt(void *map, size_t size, bool uncached_mem_test, HistogramStats &stats)
{
    DEBUG_PRINT("[DEBUG] Starting non-temporal write benchmark: size=%zu bytes\n", size);
    auto time_start = std::chrono::high_resolution_clock::now();

    volatile uint64_t *pt = static_cast<volatile uint64_t *>(map);
    size_t tsize = (size / sizeof(uint64_t));
    size_t writes = 0;

    // Create a 512-bit vector filled with our test pattern
    uint64_t pattern = 0xDEADBEEF;
    __m512i v_pattern = _mm512_set1_epi64(pattern);

    DEBUG_PRINT("[DEBUG] Writing %zu uint64_t elements with NT stores (pattern: 0x%lx)\n", tsize, pattern);
    // Process 512 bits (64 bytes) at a time using non-temporal stores
    for (size_t i = 0; i < tsize; i += 8)
    {
        _mm512_stream_si512((__m512i *)(pt + i), v_pattern);
        writes += 8;
    }
    DEBUG_PRINT("[DEBUG] Completed %zu NT writes\n", writes);

    // Ensure all non-temporal writes are complete
    _mm_sfence();

    uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::high_resolution_clock::now() - time_start)
                              .count();
    DEBUG_PRINT("[DEBUG] Elapsed time: %lu ns\n", elapsed_ns);

    stats.event(elapsed_ns);
    print_benchmark_results(uncached_mem_test, size, writes, elapsed_ns, pattern);
}

#endif // WC_BENCH_HPP