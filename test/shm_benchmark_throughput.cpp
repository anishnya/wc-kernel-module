#include <immintrin.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>  // Add this line
#include <vector>

#include "constants.h"
#include "stats.hpp"

struct alignas(CACHE_LINE_SIZE) SharedData
{
    alignas(CACHE_LINE_SIZE) std::atomic<bool> done;
    alignas(CACHE_LINE_SIZE) std::atomic<bool> ready;
    alignas(CACHE_LINE_SIZE) std::atomic<std::uint64_t> counter;
    alignas(CACHE_LINE_SIZE) volatile char buffer[];  // Flexible array member
};

// Modify the producer function to accept a specific core
void producer(SharedData *data, std::size_t buffer_size, int iterations, int thread_id, int core_id)
{
    // Pin to specified core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // For write stats
    std::string filename =
        "Regular_write__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) + 
        "__core" + std::to_string(core_id);
    HistogramStats stats(filename, 10, 100000, true);

    // For signal stats
    std::string signal_filename =
        "Signal_write__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) + 
        "__core" + std::to_string(core_id);
    HistogramStats signal_stats(signal_filename, 10, 10000000, true);

    for (int i = 0; i < iterations; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();

        const std::uint64_t pattern = i % 255;
        volatile std::uint64_t *buf_u64 = reinterpret_cast<volatile std::uint64_t *>(data->buffer);
        const std::size_t num_u64s = buffer_size / sizeof(std::uint64_t);

        for (std::size_t j = 0; j < num_u64s; j++)
        {
            buf_u64[j] = pattern;
        }

        // Record just the write time, before synchronization
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        // Signal consumer (after timing)
        auto signal_start = std::chrono::high_resolution_clock::now();
        data->counter.store(i, std::memory_order_release);
        signal_stats.event(
            std::chrono::duration_cast<std::chrono::nanoseconds>(signal_start - end).count());

        data->ready.store(true, std::memory_order_release);

        // Wait for consumer to finish but don't include in timing
        while (data->ready.load(std::memory_order_relaxed))
        {
            continue;
        }
    }

    // Signal completion
    data->done.store(true, std::memory_order_release);
}

// Modify consumer function to accept a specific core
void consumer(SharedData *data, std::size_t buffer_size, int thread_id, int core_id)
{
    // Pin to specified core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // Create stats collector with matching wc_test format
    std::string filename =
        "Regular_read__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) +
        "__core" + std::to_string(core_id);
    HistogramStats stats(filename, 10, 10000000, true);

    for (;;)
    {
        // Wait for producer to signal readiness
        while (!data->ready.load(std::memory_order_relaxed))
        {
            if (data->done.load(std::memory_order_relaxed))
            {
                return;  // Exit if producer is done
            }

            continue;
        }

        std::uint64_t current = data->counter.load(std::memory_order_acquire);

        // Measure just the read time
        auto start = std::chrono::high_resolution_clock::now();

        // Read and verify buffer contents using uint64_t for efficiency
        bool valid = true;
        const std::uint64_t expected_pattern = current % 255;
        const volatile std::uint64_t *buf_u64 =
            reinterpret_cast<const volatile std::uint64_t *>(data->buffer);
        const std::size_t num_u64s = buffer_size / sizeof(std::uint64_t);

        for (std::size_t i = 0; i < num_u64s && valid; i++)
        {
            if (buf_u64[i] != expected_pattern)
            {
                valid = false;
            }
        }

        if (!valid)
        {
            std::cout << "Buffer verification failed at iteration " << current << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        // Signal producer (after timing)
        data->ready.store(false, std::memory_order_release);
    }
}

// Similarly modify the AVX512 variants to accept core_id
void producer_avx512(SharedData *data, std::size_t buffer_size, int iterations, int thread_id, int core_id)
{
    // Pin to specified core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // For write stats
    std::string filename =
        "AVX512_write__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) + 
        "__core" + std::to_string(core_id);
    HistogramStats stats(filename, 10, 100000, true);

    // For signal stats
    std::string signal_filename =
        "Signal_write_avx512__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) + 
        "__core" + std::to_string(core_id);
    HistogramStats signal_stats(signal_filename, 10, 10000000, true);

    for (int i = 0; i < iterations; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();

        const std::uint64_t pattern = i % 255;
        // Create vector with repeated pattern
        __m512i pattern_vec = _mm512_set1_epi64(pattern);

        // Calculate number of AVX-512 vectors needed (64 bytes per vector)
        const std::size_t num_vectors = buffer_size / 64;
        volatile char *buf = data->buffer;

        // Write using AVX-512 temporal stores
        for (std::size_t j = 0; j < num_vectors; j++)
        {
            _mm512_store_si512((__m512i *)(buf + j * 64), pattern_vec);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        // Signal consumer (after timing)
        auto signal_start = std::chrono::high_resolution_clock::now();
        data->counter.store(i, std::memory_order_release);
        signal_stats.event(
            std::chrono::duration_cast<std::chrono::nanoseconds>(signal_start - end).count());

        data->ready.store(true, std::memory_order_release);

        while (data->ready.load(std::memory_order_relaxed))
        {
            continue;
        }
    }

    data->done.store(true, std::memory_order_release);
}

void consumer_avx512(SharedData *data, std::size_t buffer_size, int thread_id, int core_id)
{
    // Pin to specified core
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    std::string filename =
        "AVX512_read__" + std::to_string(buffer_size) + "__" + std::to_string(thread_id) +
        "__core" + std::to_string(core_id);
    HistogramStats stats(filename, 10, 10000000, true);

    for (;;)
    {
        while (!data->ready.load(std::memory_order_relaxed))
        {
            if (data->done.load(std::memory_order_relaxed))
            {
                return;
            }
            continue;
        }

        std::uint64_t current = data->counter.load(std::memory_order_acquire);
        auto start = std::chrono::high_resolution_clock::now();

        bool valid = true;
        const std::uint64_t expected_pattern = current % 255;
        __m512i pattern_vec = _mm512_set1_epi64(expected_pattern);

        const volatile char *buf = data->buffer;
        const std::size_t num_vectors = buffer_size / 64;

        // Read and compare using AVX-512
        for (std::size_t i = 0; i < num_vectors && valid; i++)
        {
            __m512i loaded = _mm512_load_si512((const void *)(buf + i * 64));
            __mmask8 cmp = _mm512_cmpeq_epi64_mask(loaded, pattern_vec);
            if (cmp != 0xFF)  // All 8 64-bit integers should match
            {
                valid = false;
            }
        }

        if (!valid)
        {
            std::cout << "Buffer verification failed at iteration " << current << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        data->ready.store(false, std::memory_order_release);
    }
}

// Update the wrapper functions to distribute work across cores
void producerWrapper(std::vector<SharedData *> data_ptrs, std::size_t buffer_size, int iterations,
                     int num_threads)
{
    // Use specific cores for producers: 1 and 3
    const std::vector<int> producer_cores = {1, 3};
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int i = 0; i < num_threads; i++)
    {
        // Use regular producer for multi-thread scenario
        int core_id = producer_cores[i % producer_cores.size()];
        threads.emplace_back(producer, data_ptrs[i], buffer_size, iterations, i, core_id);
    }

    // Join all threads
    for (auto &t : threads)
    {
        t.join();
    }
}

void consumerWrapper(std::vector<SharedData *> data_ptrs, std::size_t buffer_size, int num_threads)
{
    // Use specific cores for consumers: 5 and 7
    const std::vector<int> consumer_cores = {5, 7};
    
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int i = 0; i < num_threads; i++)
    {
        // Use regular consumer for multi-thread scenario
        int core_id = consumer_cores[i % consumer_cores.size()];
        threads.emplace_back(consumer, data_ptrs[i], buffer_size, i, core_id);
    }

    // Join all threads
    for (auto &t : threads)
    {
        t.join();
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <buffer_size> <num_threads>" << std::endl;
        return 1;
    }

    std::size_t buffer_size = std::stoull(argv[1]);
    int num_threads = std::stoi(argv[2]);

    if (num_threads <= 0 || num_threads > 2)
    {
        std::cerr << "Number of threads must be 1 or 2" << std::endl;
        return 1;
    }

    // Create shared memory regions with 64-byte alignment for each thread
    std::size_t total_size = sizeof(SharedData) + buffer_size;
    std::vector<void *> shm_regions;
    std::vector<SharedData *> data_ptrs;

    for (int i = 0; i < num_threads; i++)
    {
        void *shm = mmap(nullptr, total_size + 64, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        if (shm == MAP_FAILED)
        {
            perror("mmap failed");
            // Cleanup already allocated regions
            for (void *ptr : shm_regions)
            {
                munmap(ptr, total_size + 64);
            }
            return 1;
        }

        // Store raw pointer for cleanup
        shm_regions.push_back(shm);

        // Align to 64-byte boundary
        void *aligned_shm = reinterpret_cast<void *>((reinterpret_cast<uintptr_t>(shm) + 63) & ~63);

        // Initialize shared data
        SharedData *data = new (aligned_shm) SharedData();
        data->ready.store(false);
        data->done.store(false);
        data->counter.store(0);

        data_ptrs.push_back(data);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        // Child process (consumers)
        consumerWrapper(data_ptrs, buffer_size, num_threads);
        exit(0);
    }
    else
    {
        // Parent process (producers)
        producerWrapper(data_ptrs, buffer_size, NUM_ITERS / (std::size_t)num_threads, num_threads);
        wait(nullptr);
    }

    // Cleanup at end
    for (void *ptr : shm_regions)
    {
        munmap(ptr, total_size + 64);
    }

    return 0;
}