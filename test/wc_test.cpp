#include <assert.h>
#include <fcntl.h>
#include <immintrin.h>  // Add this at the top for _mm_clflushopt
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>  // Add this for uint64_t
#include <iomanip>  // for std::fixed and std::setprecision
#include <iostream>
#include <vector>

#include "bench.hpp"
#include "constants.h"
#include "mem.hpp"
#include "stats.hpp"

void usage(int ac, char **av)
{
    std::cout << "usage: " << av[0]
              << " cached|uncached mem_dev temporal_flush non_temporal_flush\n";
    std::cout << "       temporal_flush: 0|1 (disable|enable temporal cache flush)\n";
    std::cout << "       non_temporal_flush: 0|1 (disable|enable non-temporal cache flush)\n";
    exit(1);
}

void die(const char *msg)
{
    std::cout << msg << std::endl;
    exit(1);
}

const std::uint64_t testBucketSize(const std::size_t buffer_size)
{
    if (buffer_size < BYTES_PER_KB)
    {
        return 1;  // 1ns bucket for byte-sized buffers
    }
    else if (buffer_size < BYTES_PER_MB)
    {
        return 10;  // 10ns bucket for KB-sized buffers
    }
    else
    {
        return 200;  // 200ns bucket for MB-sized buffers
    }
}

const std::string testFileName(const std::string test_type, const std::size_t message_size,
                               const std::size_t batch_size)
{
    return test_type + "_" + std::to_string(message_size) + "_" + std::to_string(batch_size);
}

const std::size_t numItersBatchSize(const std::size_t batch_size) { return NUM_ITERS / batch_size; }

void copyMixedTests(const MemArena &src_arena, const MemArena &copy_arena,
                    const std::size_t message_size, const std::size_t batch_size)
{
    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_copy_nt_src(testFileName("NT Copy Src", message_size, batch_size),
                                     bucket_size, HISTO_SIZE, true);
    HistogramStats stats_copy_nt_dst(testFileName("NT Copy", message_size, batch_size), bucket_size,
                                     HISTO_SIZE, true);

    void *src_ptr = src_arena.ptr;
    void *dst_ptr = copy_arena.ptr;

    bool src_cache_flush = static_cast<bool>(src_arena.flush_type);
    bool dst_cache_flush = static_cast<bool>(copy_arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmarkNtSrc(src_ptr, dst_ptr, message_size, batch_size, stats_copy_nt_src,
                           dst_cache_flush);
    }

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmarkNtDst(src_ptr, dst_ptr, message_size, batch_size, stats_copy_nt_dst,
                           src_cache_flush);
    }

    return;
}

void runRegularCopyTests(const MemArena &src_arena, const MemArena &copy_arena,
                         const std::size_t message_size, const std::size_t batch_size)
{
    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_copy(testFileName("Copy Fast", message_size, batch_size), bucket_size,
                              HISTO_SIZE, true);

    void *src_ptr = src_arena.ptr;
    void *dst_ptr = copy_arena.ptr;

    bool src_cache_flush = static_cast<bool>(src_arena.flush_type);
    bool dst_cache_flush = static_cast<bool>(copy_arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmark(src_ptr, dst_ptr, message_size, batch_size, stats_copy, src_cache_flush,
                      dst_cache_flush);
    }
}

void runNtCopyTests(const MemArena &src_arena, const MemArena &copy_arena,
                    const std::size_t message_size, const std::size_t batch_size)
{
    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_copy_nt(testFileName("NT Both", message_size, batch_size), bucket_size,
                                 HISTO_SIZE, true);

    void *src_ptr = src_arena.ptr;
    void *dst_ptr = copy_arena.ptr;
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmarkNtSrcDst(src_ptr, dst_ptr, message_size, batch_size, stats_copy_nt);
    }
}

void runNtSrcCopyTests(const MemArena &src_arena, const MemArena &copy_arena,
                       const std::size_t message_size, const std::size_t batch_size)
{
    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_copy_nt_src(testFileName("NT Copy Src", message_size, batch_size),
                                     bucket_size, HISTO_SIZE, true);

    void *src_ptr = src_arena.ptr;
    void *dst_ptr = copy_arena.ptr;
    bool dst_cache_flush = static_cast<bool>(copy_arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmarkNtSrc(src_ptr, dst_ptr, message_size, batch_size, stats_copy_nt_src,
                           dst_cache_flush);
    }
}

void runNtDstCopyTests(const MemArena &src_arena, const MemArena &copy_arena,
                       const std::size_t message_size, const std::size_t batch_size)
{
    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_copy_nt_dst(testFileName("NT Copy", message_size, batch_size), bucket_size,
                                     HISTO_SIZE, true);

    void *src_ptr = src_arena.ptr;
    void *dst_ptr = copy_arena.ptr;
    bool src_cache_flush = static_cast<bool>(src_arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        copyBenchmarkNtDst(src_ptr, dst_ptr, message_size, batch_size, stats_copy_nt_dst,
                           src_cache_flush);
    }
}

void copyIteration(char *dev, const std::size_t message_size, const std::size_t batch_size,
                   const CacheFlushType temporal_flush, const CacheFlushType non_temporal_flush)
{
    DEBUG_PRINT("Starting Copy benchmark iteration: size="
                << message_size << " bytes, batch_size=" << batch_size
                << " (mode=" << (mem_type == MemoryType::WC ? "uncached" : "cached") << ")");

    const std::size_t buffer_size = message_size * batch_size;
    MemArena wc_mem_arena(dev, buffer_size, MemoryType::WC, non_temporal_flush);
    MemArena wb_mem_arena(dev, buffer_size, MemoryType::WB, temporal_flush);

    MemArena copy_wc_mem_arena(dev, buffer_size, MemoryType::WC, non_temporal_flush);
    MemArena copy_wb_mem_arena(dev, buffer_size, MemoryType::WB, temporal_flush);

    // Both WB Backed
    runRegularCopyTests(wb_mem_arena, copy_wb_mem_arena, message_size, batch_size);
    runNtCopyTests(wb_mem_arena, copy_wb_mem_arena, message_size, batch_size);

    // Both WC Backed
    runNtCopyTests(wc_mem_arena, copy_wc_mem_arena, message_size, batch_size);

    // WC Source, WB Destination
    runNtSrcCopyTests(wc_mem_arena, copy_wb_mem_arena, message_size, batch_size);

    // WB Source, WC Destination
    runNtDstCopyTests(wb_mem_arena, copy_wc_mem_arena, message_size, batch_size);

    DEBUG_PRINT("Completed all RW benchmarks for iteration");

    return;
}

void runRwTests(const MemArena &arena, const std::size_t message_size, const std::size_t batch_size,
                HistogramStats &stats_read, HistogramStats &stats_write)
{
    DEBUG_PRINT("Running RW tests: size=" << message_size << " bytes, batch_size=" << batch_size
                                          << " (mode=" << (is_uncached ? "uncached" : "cached")
                                          << ")");

    void *ptr = arena.ptr;
    bool cache_flush = static_cast<bool>(arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        writeBenchmark(ptr, message_size, batch_size, stats_write, cache_flush);
    }

    // Skip regular reads for large uncached buffers
    if (!(arena.type == MemoryType::WC && message_size > 1 * BYTES_PER_KB))
    {
        for (std::size_t i = 0; i < iter_batch_size; i++)
        {
            readBenchmark(ptr, message_size, batch_size, stats_read, cache_flush);
        }
    }

    return;
}

void runRwTestsAvx512(const MemArena &arena, const std::size_t message_size,
                      const std::size_t batch_size, HistogramStats &stats_read,
                      HistogramStats &stats_write)
{
    DEBUG_PRINT("Running AVX-512 RW tests: size="
                << message_size << " bytes, batch_size=" << batch_size
                << " (type=" << (arena.type == MemoryType::WC ? "WC" : "WB") << ")");

    void *ptr = arena.ptr;
    bool cache_flush = static_cast<bool>(arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        writeBenchmarkAvx512(ptr, message_size, batch_size, stats_write, cache_flush);
    }

    // Skip regular reads for large uncached buffers
    if (!(arena.type == MemoryType::WC && message_size > 1 * BYTES_PER_KB))
    {
        for (std::size_t i = 0; i < iter_batch_size; i++)
        {
            readBenchmarkAvx512(ptr, message_size, batch_size, stats_read, cache_flush);
        }
    }

    return;
}

void runRwTestsAvx512Unrolled(const MemArena &arena, const std::size_t message_size,
                              const std::size_t batch_size, HistogramStats &stats_read,
                              HistogramStats &stats_write)
{
    DEBUG_PRINT("Running AVX-512 Unrolled RW tests: size="
                << message_size << " bytes, batch_size=" << batch_size
                << " (type=" << (arena.type == MemoryType::WC ? "WC" : "WB") << ")");

    void *ptr = arena.ptr;
    bool cache_flush = static_cast<bool>(arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        writeBenchmarkAvx512Unrolled(ptr, message_size, batch_size, stats_write, cache_flush);
    }

    // Skip regular reads for large uncached buffers
    if (!(arena.type == MemoryType::WC && message_size > 1 * BYTES_PER_KB))
    {
        for (std::size_t i = 0; i < iter_batch_size; i++)
        {
            readBenchmarkAvx512Unrolled(ptr, message_size, batch_size, stats_read, cache_flush);
        }
    }
}

void runNtRWTests(const MemArena &arena, const std::size_t message_size,
                  const std::size_t batch_size, HistogramStats &stats_read_nt,
                  HistogramStats &stats_write_nt)
{
    DEBUG_PRINT("Running NT RW tests: size=" << message_size
                                             << " bytes, batch_size=" << batch_size);

    void *ptr = arena.ptr;
    bool cache_flush = static_cast<bool>(arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        writeBenchmarkNt(ptr, message_size, batch_size, stats_write_nt, cache_flush);
    }

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        readBenchmarkNt(ptr, message_size, batch_size, stats_read_nt, cache_flush);
    }

    return;
}

void runNtRWTestsUnrolled(const MemArena &arena, const std::size_t message_size,
                          const std::size_t batch_size, HistogramStats &stats_read_nt,
                          HistogramStats &stats_write_nt)
{
    DEBUG_PRINT("Running NT Unrolled RW tests: size=" << message_size
                                                      << " bytes, batch_size=" << batch_size);

    void *ptr = arena.ptr;
    bool cache_flush = static_cast<bool>(arena.flush_type);
    const std::size_t iter_batch_size = numItersBatchSize(batch_size);

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        writeBenchmarkNtUnrolled(ptr, message_size, batch_size, stats_write_nt, cache_flush);
    }

    for (std::size_t i = 0; i < iter_batch_size; i++)
    {
        readBenchmarkNtUnrolled(ptr, message_size, batch_size, stats_read_nt, cache_flush);
    }
}

void runRwIteration(char *dev, const std::size_t message_size, const std::size_t batch_size,
                    const MemoryType mem_type, const CacheFlushType temporal_flush,
                    const CacheFlushType non_temporal_flush)
{
    DEBUG_PRINT("Starting RW benchmark iteration: size="
                << message_size << " bytes, batch_size=" << batch_size
                << " (mode=" << (mem_type == MemoryType::WC ? "uncached" : "cached") << ")");

    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_read(testFileName("Regular Read", message_size, batch_size), bucket_size,
                              HISTO_SIZE, true);
    HistogramStats stats_read_nt(testFileName("NT Read", message_size, batch_size), bucket_size,
                                 HISTO_SIZE, true);
    HistogramStats stats_write(testFileName("Regular Write", message_size, batch_size), bucket_size,
                               HISTO_SIZE, true);
    HistogramStats stats_write_nt(testFileName("NT Write", message_size, batch_size), bucket_size,
                                  HISTO_SIZE, true);
    HistogramStats stats_read_avx512(testFileName("AVX512 Read", message_size, batch_size),
                                     bucket_size, HISTO_SIZE, true);
    HistogramStats stats_write_avx512(testFileName("AVX512 Write", message_size, batch_size),
                                      bucket_size, HISTO_SIZE, true);

    const std::size_t buffer_size = message_size * batch_size;
    const CacheFlushType arena_cache_policy =
        (mem_type == MemoryType::WC) ? non_temporal_flush : temporal_flush;
    MemArena mem_arena(dev, buffer_size, mem_type, arena_cache_policy);

    runRwTests(mem_arena, message_size, batch_size, stats_read, stats_write);
    runNtRWTests(mem_arena, message_size, batch_size, stats_read_nt, stats_write_nt);
    runRwTestsAvx512(mem_arena, message_size, batch_size, stats_read_avx512, stats_write_avx512);

    DEBUG_PRINT("Completed all RW benchmarks for iteration");

    return;
}

void runRwIterationUnrolled(char *dev, const std::size_t message_size, const std::size_t batch_size,
                            const MemoryType mem_type, const CacheFlushType temporal_flush,
                            const CacheFlushType non_temporal_flush)
{
    DEBUG_PRINT("Starting Unrolled RW benchmark iteration: size="
                << message_size << " bytes, batch_size=" << batch_size
                << " (mode=" << (mem_type == MemoryType::WC ? "uncached" : "cached") << ")");

    const std::uint64_t bucket_size = testBucketSize(message_size);
    HistogramStats stats_read_unrolled(
        testFileName("Regular Read Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE,
        true);
    HistogramStats stats_read_nt_unrolled(
        testFileName("NT Read Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE, true);
    HistogramStats stats_write_unrolled(
        testFileName("Regular Write Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE,
        true);
    HistogramStats stats_write_nt_unrolled(
        testFileName("NT Write Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE, true);
    HistogramStats stats_read_avx512_unrolled(
        testFileName("AVX512 Read Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE,
        true);
    HistogramStats stats_write_avx512_unrolled(
        testFileName("AVX512 Write Unrolled", message_size, batch_size), bucket_size, HISTO_SIZE,
        true);

    const std::size_t buffer_size = message_size * batch_size;
    const CacheFlushType arena_cache_policy =
        (mem_type == MemoryType::WC) ? non_temporal_flush : temporal_flush;
    MemArena mem_arena(dev, buffer_size, mem_type, arena_cache_policy);

    // Skip if message size isn't aligned to unroll factor * vector size
    const std::size_t min_size = sizeof(__m512i) * UNROLL_FACTOR;
    if (message_size % min_size == 0)
    {
        runRwTestsAvx512Unrolled(mem_arena, message_size, batch_size, stats_read_avx512_unrolled,
                                 stats_write_avx512_unrolled);
        runNtRWTestsUnrolled(mem_arena, message_size, batch_size, stats_read_nt_unrolled,
                             stats_write_nt_unrolled);
        DEBUG_PRINT("Completed all unrolled RW benchmarks for iteration");
    }
    else
    {
        DEBUG_PRINT("Skipping unrolled benchmarks - message size "
                    << message_size << " not aligned to " << min_size << " bytes");
    }

    return;
}

int main(int ac, char **av)
{
    // Pin process to core 3
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1)
    {
        DEBUG_PRINT("Failed to set CPU affinity");
        die("Failed to set CPU affinity");
    }

    if (ac != 5)  // Updated to check for 5 arguments
    {
        usage(ac, av);
    }

    std::cout << "Device path: " << av[2] << std::endl;

    MemoryType mem_type;
    if (!strcmp(av[1], "uncached"))
    {
        mem_type = MemoryType::WC;
    }
    else if (!strcmp(av[1], "cached"))
    {
        mem_type = MemoryType::WB;
    }
    else
    {
        usage(ac, av);
    }

    const CacheFlushType temporal_flush = static_cast<CacheFlushType>(atoi(av[3]));
    const CacheFlushType non_temporal_flush = static_cast<CacheFlushType>(atoi(av[4]));

    if (static_cast<int>(temporal_flush) != 0 && static_cast<int>(temporal_flush) != 1)
    {
        std::cout << "Error: temporal_flush must be 0 or 1" << std::endl;
        usage(ac, av);
    }

    if (static_cast<int>(non_temporal_flush) != 0 && static_cast<int>(non_temporal_flush) != 1)
    {
        std::cout << "Error: non_temporal_flush must be 0 or 1" << std::endl;
        usage(ac, av);
    }

    for (const std::size_t message_size : BUFFER_SIZES)
    {
        for (const std::size_t batch_size : BATCH_SIZES)
        {
            DEBUG_PRINT("Running benchmarks for size=" << message_size
                                                       << " bytes and batch_size=" << batch_size);
            runRwIteration(av[2], message_size, batch_size, mem_type, temporal_flush,
                           non_temporal_flush);
            // runRwIterationUnrolled(av[2], message_size, batch_size, mem_type, temporal_flush,
            //                        non_temporal_flush);
        }
    }

    // for (const std::size_t message_size : BUFFER_SIZES)
    // {
    //     for (const std::size_t batch_size : BATCH_SIZES)
    //     {
    //         DEBUG_PRINT("Running benchmarks for size=" << message_size
    //                                                    << " bytes and batch_size=" <<
    //                                                    batch_size);
    //         copyIteration(av[2], message_size, batch_size, temporal_flush, non_temporal_flush);
    //     }
    // }

    return 0;
}
