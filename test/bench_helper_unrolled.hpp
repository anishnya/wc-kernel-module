#ifndef WC_BENCH_HELPER_UNROLLED_HPP
#define WC_BENCH_HELPER_UNROLLED_HPP

#include <immintrin.h>

#include <cstdint>

#include "constants.h"

/*
    Read functions - Unrolled x8
*/
void read_buffer_avx512_unrolled(void *ptr, const std::size_t size,
                                 const std::uint64_t expected_pattern)
{
    const char *buf = static_cast<const char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vector_blocks = size / (vec_size * UNROLL_FACTOR);
    const __m512i expected = _mm512_set1_epi64(expected_pattern);

    // Process 8 vectors per iteration
    for (std::size_t i = 0; i < num_vector_blocks; i++)
    {
        const char *block_ptr = buf + i * vec_size * UNROLL_FACTOR;
        __m512i loaded[UNROLL_FACTOR];
        __mmask8 comparison[UNROLL_FACTOR];

        // Interleaved loads and comparisons to improve instruction-level parallelism
        loaded[0] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 0));
        loaded[4] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 4));
        comparison[0] = _mm512_cmpeq_epi64_mask(loaded[0], expected);

        loaded[1] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 1));
        loaded[5] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 5));
        comparison[4] = _mm512_cmpeq_epi64_mask(loaded[4], expected);

        loaded[2] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 2));
        loaded[6] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 6));
        comparison[1] = _mm512_cmpeq_epi64_mask(loaded[1], expected);

        loaded[3] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 3));
        loaded[7] = _mm512_load_si512((__m512i *)(block_ptr + vec_size * 7));
        comparison[5] = _mm512_cmpeq_epi64_mask(loaded[5], expected);

        comparison[2] = _mm512_cmpeq_epi64_mask(loaded[2], expected);
        comparison[6] = _mm512_cmpeq_epi64_mask(loaded[6], expected);
        comparison[3] = _mm512_cmpeq_epi64_mask(loaded[3], expected);
        comparison[7] = _mm512_cmpeq_epi64_mask(loaded[7], expected);

        // Check all comparisons
        for (std::size_t j = 0; j < UNROLL_FACTOR; j++)
        {
            if (comparison[j] != ALL_EQUAL_MASK)
            {
                DEBUG_PRINT("[DEBUG] Vector verification failed at block " << i << " vector " << j);
                std::exit(1);
            }
        }
    }

    return;
}

void read_buffer_nt_unrolled(void *ptr, const std::size_t size,
                             const std::uint64_t expected_pattern)
{
    const char *buf = static_cast<const char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vector_blocks = size / (vec_size * UNROLL_FACTOR);
    const __m512i expected = _mm512_set1_epi64(expected_pattern);

    for (std::size_t i = 0; i < num_vector_blocks; i++)
    {
        const char *block_ptr = buf + i * vec_size * UNROLL_FACTOR;
        __m512i loaded[UNROLL_FACTOR];
        __mmask8 comparison[UNROLL_FACTOR];

        // Interleaved NT loads and comparisons
        loaded[0] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 0));
        loaded[4] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 4));
        comparison[0] = _mm512_cmpeq_epi64_mask(loaded[0], expected);

        loaded[1] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 1));
        loaded[5] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 5));
        comparison[4] = _mm512_cmpeq_epi64_mask(loaded[4], expected);

        loaded[2] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 2));
        loaded[6] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 6));
        comparison[1] = _mm512_cmpeq_epi64_mask(loaded[1], expected);

        loaded[3] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 3));
        loaded[7] = _mm512_stream_load_si512((__m512i *)(block_ptr + vec_size * 7));
        comparison[5] = _mm512_cmpeq_epi64_mask(loaded[5], expected);

        comparison[2] = _mm512_cmpeq_epi64_mask(loaded[2], expected);
        comparison[6] = _mm512_cmpeq_epi64_mask(loaded[6], expected);
        comparison[3] = _mm512_cmpeq_epi64_mask(loaded[3], expected);
        comparison[7] = _mm512_cmpeq_epi64_mask(loaded[7], expected);

        for (std::size_t j = 0; j < UNROLL_FACTOR; j++)
        {
            if (comparison[j] != ALL_EQUAL_MASK)
            {
                DEBUG_PRINT("[DEBUG] Vector verification failed at block " << i << " vector " << j);
                std::exit(1);
            }
        }
    }

    return;
}

/*
    Write functions - Unrolled x8
*/
void write_buffer_avx512_unrolled(void *ptr, const std::size_t size, const std::uint64_t pattern)
{
    char *buf = static_cast<char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vector_blocks = size / (vec_size * UNROLL_FACTOR);
    const __m512i pattern_vec = _mm512_set1_epi64(pattern);

    for (std::size_t i = 0; i < num_vector_blocks; i++)
    {
        char *block_ptr = buf + i * vec_size * UNROLL_FACTOR;

        // Interleaved stores for better instruction-level parallelism
        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 0), pattern_vec);
        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 4), pattern_vec);

        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 1), pattern_vec);
        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 5), pattern_vec);

        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 2), pattern_vec);
        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 6), pattern_vec);

        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 3), pattern_vec);
        _mm512_store_si512((__m512i *)(block_ptr + vec_size * 7), pattern_vec);
    }

    return;
}

void write_buffer_nt_unrolled(void *ptr, const std::size_t size, const std::uint64_t pattern)
{
    char *buf = static_cast<char *>(ptr);
    const std::size_t vec_size = sizeof(__m512i);
    const std::size_t num_vector_blocks = size / (vec_size * UNROLL_FACTOR);
    const __m512i pattern_vec = _mm512_set1_epi64(pattern);

    for (std::size_t i = 0; i < num_vector_blocks; i++)
    {
        char *block_ptr = buf + i * vec_size * UNROLL_FACTOR;

        // Unrolled NT stores
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 0), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 1), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 2), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 3), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 4), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 5), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 6), pattern_vec);
        _mm512_stream_si512((__m512i *)(block_ptr + vec_size * 7), pattern_vec);
    }

    return;
}

#endif  // WC_BENCH_HELPER_UNROLLED_HPP