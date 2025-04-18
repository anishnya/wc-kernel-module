#ifndef WC_BENCH_HPP
#define WC_BENCH_HPP

#include <immintrin.h>

#include <chrono>
#include <cstring>  // For memcpy

#include "bench_helper.hpp"
#include "bench_helper_unrolled.hpp"
#include "constants.h"
#include "stats.hpp"

/*
    Read benchmark functions
*/

void readBenchmark(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                   HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting Regular Read benchmark: size=" << message_size
                                                         << " bytes, batch_size=" << batch_size);

    // Intial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        read_buffer(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }

    return;
}

void readBenchmarkNt(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                     HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting NT Read benchmark: size=" << message_size
                                                    << " bytes, batch_size=" << batch_size);

    // Flushing should not impact performance
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        read_buffer_nt(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }

    return;
}

void readBenchmarkAvx512(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                         HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting AVX-512 Read benchmark: size=" << message_size
                                                         << " bytes, batch_size=" << batch_size);

    // Initial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        read_buffer_avx512(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }

    return;
}

void readBenchmarkAvx512Unrolled(void* ptr, const std::size_t message_size,
                                 const std::size_t batch_size, HistogramStats& stats,
                                 const bool cache_flush)
{
    DEBUG_PRINT("Starting AVX-512 Unrolled Read benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    // Initial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        read_buffer_avx512_unrolled(reinterpret_cast<void*>(current_ptr), message_size,
                                    TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }
}

/*
    Write benchmark functions
*/

void writeBenchmark(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                    HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting Regular Write benchmark: size=" << message_size
                                                          << " bytes, batch_size=" << batch_size);

    // Intial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        write_buffer(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }

    return;
}

void writeBenchmarkNt(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                      HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting NT Write benchmark: size=" << message_size
                                                     << " bytes, batch_size=" << batch_size);

    // Flushing should not impact performance
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    auto time_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);
        write_buffer_nt(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        current_ptr += message_size;
    }

    // Forced flush amortized across larger sizes
    _mm_sfence();

    // Stats across the entire batch
    const std::uint64_t total_elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - time_start)
            .count();
    const std::uint64_t elapsed_ns = total_elapsed_ns / batch_size;

    for (std::size_t i = 0; i < batch_size; i++)
    {
        stats.event(elapsed_ns);
    }

    return;
}

void writeBenchmarkAvx512(void* ptr, const std::size_t message_size, const std::size_t batch_size,
                          HistogramStats& stats, const bool cache_flush)
{
    DEBUG_PRINT("Starting AVX-512 Write benchmark: size=" << message_size
                                                          << " bytes, batch_size=" << batch_size);

    // Initial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        write_buffer_avx512(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }

    return;
}

void writeBenchmarkAvx512Unrolled(void* ptr, const std::size_t message_size,
                                  const std::size_t batch_size, HistogramStats& stats,
                                  const bool cache_flush)
{
    DEBUG_PRINT("Starting AVX-512 Unrolled Write benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    // Initial cache miss amortized over larger sizes
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        write_buffer_avx512_unrolled(reinterpret_cast<void*>(current_ptr), message_size,
                                     TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }
}

void readBenchmarkNtUnrolled(void* ptr, const std::size_t message_size,
                             const std::size_t batch_size, HistogramStats& stats,
                             const bool cache_flush)
{
    DEBUG_PRINT("Starting NT Unrolled Read benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    // Flushing should not impact performance
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        read_buffer_nt_unrolled(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_ptr += message_size;
    }
}

void writeBenchmarkNtUnrolled(void* ptr, const std::size_t message_size,
                              const std::size_t batch_size, HistogramStats& stats,
                              const bool cache_flush)
{
    DEBUG_PRINT("Starting NT Unrolled Write benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    // Flushing should not impact performance
    if (cache_flush)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(ptr, flush_size, true);
    }

    std::uintptr_t current_ptr = reinterpret_cast<std::uintptr_t>(ptr);

    auto time_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " at address 0x" << std::hex
                                     << current_ptr << std::dec);
        write_buffer_nt_unrolled(reinterpret_cast<void*>(current_ptr), message_size, TEST_PATTERN);
        current_ptr += message_size;
    }

    // Forced flush amortized across larger sizes
    _mm_sfence();

    const std::uint64_t total_elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - time_start)
            .count();
    const std::uint64_t elapsed_ns = total_elapsed_ns / batch_size;

    for (std::size_t i = 0; i < batch_size; i++)
    {
        stats.event(elapsed_ns);
    }
}

void copyBenchmark(void* src, void* dst, const std::size_t message_size,
                   const std::size_t batch_size, HistogramStats& stats, const bool cache_flush_src,
                   const bool cache_flush_dst)
{
    DEBUG_PRINT("Starting Regular Copy benchmark: size=" << message_size
                                                         << " bytes, batch_size=" << batch_size);

    // Intial cache miss amortized over larger sizes
    if (cache_flush_src)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(src, flush_size, true);
    }

    if (cache_flush_dst)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(dst, flush_size, true);
    }

    std::uintptr_t current_src_ptr = reinterpret_cast<std::uintptr_t>(src);
    std::uintptr_t current_dst_ptr = reinterpret_cast<std::uintptr_t>(dst);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " src=0x" << std::hex
                                     << current_src_ptr << " dst=0x" << current_dst_ptr
                                     << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        copy_buffer_fast(reinterpret_cast<void*>(current_dst_ptr),
                         reinterpret_cast<void*>(current_src_ptr), message_size);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_src_ptr += message_size;
        current_dst_ptr += message_size;
    }

    // Read the destination buffer to ensure it is not optimized away
    read_buffer(reinterpret_cast<void*>(current_dst_ptr), message_size, TEST_PATTERN);

    return;
}

void copyBenchmarkNtSrc(void* src, void* dst, const std::size_t message_size,
                        const std::size_t batch_size, HistogramStats& stats,
                        const bool cache_flush_dst)
{
    DEBUG_PRINT("Starting NT Source Copy benchmark: size=" << message_size
                                                           << " bytes, batch_size=" << batch_size);

    // Intial cache miss amortized over larger sizes
    if (cache_flush_dst)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(dst, flush_size, true);
    }

    std::uintptr_t current_src_ptr = reinterpret_cast<std::uintptr_t>(src);
    std::uintptr_t current_dst_ptr = reinterpret_cast<std::uintptr_t>(dst);

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " src=0x" << std::hex
                                     << current_src_ptr << " dst=0x" << current_dst_ptr
                                     << std::dec);

        auto time_start = std::chrono::high_resolution_clock::now();
        copy_buffer_nt_src(reinterpret_cast<void*>(current_dst_ptr),
                           reinterpret_cast<void*>(current_src_ptr), message_size);
        const std::uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             std::chrono::high_resolution_clock::now() - time_start)
                                             .count();
        stats.event(elapsed_ns);

        current_src_ptr += message_size;
        current_dst_ptr += message_size;
    }

    // Read the destination buffer to ensure it is not optimized away
    read_buffer(reinterpret_cast<void*>(current_dst_ptr), message_size, TEST_PATTERN);

    return;
}

void copyBenchmarkNtDst(void* src, void* dst, const std::size_t message_size,
                        const std::size_t batch_size, HistogramStats& stats,
                        const bool cache_flush_src)
{
    DEBUG_PRINT("Starting NT Destination Copy benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    // Intial cache miss amortized over larger sizes
    if (cache_flush_src)
    {
        const std::size_t flush_size = message_size * batch_size;
        flush_cache(src, flush_size, true);
    }

    std::uintptr_t current_src_ptr = reinterpret_cast<std::uintptr_t>(src);
    std::uintptr_t current_dst_ptr = reinterpret_cast<std::uintptr_t>(dst);

    auto time_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " src=0x" << std::hex
                                     << current_src_ptr << " dst=0x" << current_dst_ptr
                                     << std::dec);
        copy_buffer_nt_src(reinterpret_cast<void*>(current_dst_ptr),
                           reinterpret_cast<void*>(current_src_ptr), message_size);
        current_src_ptr += message_size;
        current_dst_ptr += message_size;
    }

    // Forced flush amortized across larger sizes
    _mm_sfence();

    const std::uint64_t total_elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - time_start)
            .count();
    const std::uint64_t elapsed_ns = total_elapsed_ns / batch_size;

    for (std::size_t i = 0; i < batch_size; i++)
    {
        stats.event(elapsed_ns);
    }

    return;
}

void copyBenchmarkNtSrcDst(void* src, void* dst, const std::size_t message_size,
                           const std::size_t batch_size, HistogramStats& stats)
{
    DEBUG_PRINT("Starting NT Source/Destination Copy benchmark: size="
                << message_size << " bytes, batch_size=" << batch_size);

    std::uintptr_t current_src_ptr = reinterpret_cast<std::uintptr_t>(src);
    std::uintptr_t current_dst_ptr = reinterpret_cast<std::uintptr_t>(dst);

    auto time_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < batch_size; i++)
    {
        DEBUG_PRINT("Running batch " << i << "/" << batch_size << " src=0x" << std::hex
                                     << current_src_ptr << " dst=0x" << current_dst_ptr
                                     << std::dec);
        copy_buffer_nt_both(reinterpret_cast<void*>(current_dst_ptr),
                            reinterpret_cast<void*>(current_src_ptr), message_size);
        current_src_ptr += message_size;
        current_dst_ptr += message_size;
    }

    // Forced flush amortized across larger sizes
    _mm_sfence();

    const std::uint64_t total_elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - time_start)
            .count();
    const std::uint64_t elapsed_ns = total_elapsed_ns / batch_size;

    for (std::size_t i = 0; i < batch_size; i++)
    {
        stats.event(elapsed_ns);
    }

    return;
}

#endif  // WC_BENCH_HPP