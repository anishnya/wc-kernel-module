#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <atomic>
#include <iostream>
#include <vector>
#include <cstring>
#include <immintrin.h>
#include "stats.hpp"
#include "constants.h"

struct SharedData
{
    std::atomic<bool> ready;
    std::atomic<bool> done;
    std::atomic<uint64_t> counter;
    char buffer[]; // Flexible array member
};

void producer(SharedData *data, size_t buffer_size, int iterations)
{
    // Pin to core 3
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // Create stats collector with matching wc_test format
    std::string filename = "Regular_write__" + std::to_string(buffer_size);
    HistogramStats stats(filename, 100, 10000, true);

    for (int i = 0; i < iterations; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();

        // Write pattern to buffer
        memset(data->buffer, i % 255, buffer_size);

        // Record just the write time, before synchronization
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        // Signal consumer (after timing)
        data->counter.store(i, std::memory_order_release);
        data->ready.store(true, std::memory_order_release);

        // Wait for consumer to finish but don't include in timing
        while (data->ready.load(std::memory_order_acquire))
        {
            _mm_pause();
        }
    }

    // Signal completion
    data->done.store(true, std::memory_order_release);
    // Stats will be printed automatically on destruction
}

void consumer(SharedData *data, size_t buffer_size)
{
    // Pin to core 4
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // Create stats collector with matching wc_test format
    std::string filename = "Regular_read__" + std::to_string(buffer_size);
    HistogramStats stats(filename, 100, 10000, true);

    uint64_t last_count = 0;
    while (!data->done.load(std::memory_order_acquire))
    {
        if (data->ready.load(std::memory_order_acquire))
        {
            uint64_t current = data->counter.load(std::memory_order_acquire);
            if (current != last_count + 1 && last_count != 0)
            {
                std::cerr << "Missing iteration: " << last_count << " -> " << current << std::endl;
            }
            last_count = current;

            // Measure just the read time
            auto start = std::chrono::high_resolution_clock::now();

            // Read and verify buffer contents
            bool valid = true;
            char expected = current % 255;
            for (size_t i = 0; i < buffer_size; i++)
            {
                if (data->buffer[i] != expected)
                {
                    valid = false;
                    break;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            stats.event(duration.count());

            if (!valid)
            {
                std::cerr << "Buffer verification failed at iteration " << current << std::endl;
            }

            // Signal producer (after timing)
            data->ready.store(false, std::memory_order_release);
        }
    }
}

void producer_nt(SharedData *data, size_t buffer_size, int iterations)
{
    // Pin to core 3
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // Create stats collector with matching wc_test format
    std::string filename = "NT_write__" + std::to_string(buffer_size);
    HistogramStats stats(filename, 100, 10000, true);

    // Ensure buffer is 64-byte aligned for AVX-512
    char *buf = data->buffer; // Changed to char*
    size_t vec_size = sizeof(__m512i);
    size_t num_vectors = buffer_size / vec_size;

    for (int i = 0; i < iterations; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();

        // Write pattern using non-temporal stores
        __m512i pattern = _mm512_set1_epi8(i % 255);
        for (size_t j = 0; j < num_vectors; j++)
        {
            _mm512_stream_si512((__m512i *)(buf + j * vec_size), pattern);
        }

        // Ensure NT stores are visible
        _mm_sfence();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        stats.event(duration.count());

        // Signal consumer (after timing)
        data->counter.store(i, std::memory_order_release);
        data->ready.store(true, std::memory_order_release);

        while (data->ready.load(std::memory_order_acquire))
        {
            _mm_pause();
        }
    }

    data->done.store(true, std::memory_order_release);
}

void consumer_nt(SharedData *data, size_t buffer_size)
{
    // Pin to core 4
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    // Create stats collector with matching wc_test format
    std::string filename = "NT_read__" + std::to_string(buffer_size);
    HistogramStats stats(filename, 100, 10000, true);

    // Ensure buffer is 64-byte aligned for AVX-512
    char *buf = data->buffer; // Changed to char*
    size_t vec_size = sizeof(__m512i);
    size_t num_vectors = buffer_size / vec_size;

    uint64_t last_count = 0;
    while (!data->done.load(std::memory_order_acquire))
    {
        if (data->ready.load(std::memory_order_acquire))
        {
            uint64_t current = data->counter.load(std::memory_order_acquire);
            if (current != last_count + 1 && last_count != 0)
            {
                std::cerr << "Missing iteration: " << last_count << " -> " << current << std::endl;
            }
            last_count = current;

            auto start = std::chrono::high_resolution_clock::now();

            // Read and verify using non-temporal loads
            bool valid = true;
            __m512i expected = _mm512_set1_epi8(current % 255);

            for (size_t j = 0; j < num_vectors && valid; j++)
            {
                __m512i loaded = _mm512_stream_load_si512((__m512i *)(buf + j * vec_size));
                if (!_mm512_cmpeq_epi8_mask(loaded, expected))
                {
                    valid = false;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            stats.event(duration.count());

            if (!valid)
            {
                std::cerr << "Buffer verification failed at iteration " << current << std::endl;
            }

            data->ready.store(false, std::memory_order_release);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <buffer_size>" << std::endl;
        return 1;
    }

    size_t buffer_size = std::stoull(argv[1]);

    // Create shared memory region with 64-byte alignment
    size_t total_size = sizeof(SharedData) + buffer_size;
    void *shm = mmap(nullptr, total_size + 64,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (shm == MAP_FAILED)
    {
        perror("mmap failed");
        return 1;
    }

    // Align to 64-byte boundary
    void *aligned_shm = reinterpret_cast<void *>((reinterpret_cast<uintptr_t>(shm) + 63) & ~63);

    // Initialize shared data
    SharedData *data = new (aligned_shm) SharedData();
    data->ready.store(false);
    data->done.store(false);
    data->counter.store(0);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        // Child process (consumer)
        consumer_nt(data, buffer_size);
        exit(0);
    }
    else
    {
        // Parent process (producer)
        producer_nt(data, buffer_size, NUM_ITERS);
        wait(nullptr);
    }

    munmap(shm, total_size + 64); // Use original pointer and adjusted size
    return 0;
}