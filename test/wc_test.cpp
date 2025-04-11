#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip> // for std::fixed and std::setprecision
#include <cstdint> // Add this for uint64_t
#include <sched.h>
#include <immintrin.h> // Add this at the top for _mm_clflushopt
#include "constants.h"
#include "stats.hpp"
#include "bench.hpp"

void usage(int ac, char **av)
{
	std::cout << "usage: " << av[0] << " cached   mem_dev\n";
	std::cout << "       " << av[0] << " uncached mem_dev\n";
	exit(1);
}

void die(const char *msg)
{
	std::cout << msg << std::endl;
	exit(1);
}

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#define PAGE_MASK (~(PAGE_SIZE - 1))

void *get_mem(char *dev, int size)
{
	assert(PAGE_SIZE != -1);

	int fd = open(dev, O_RDWR, 0);
	if (fd == -1)
	{
		die("couldn't open device");
	}

	if (size & ~PAGE_MASK)
	{
		size = (size & PAGE_MASK) + PAGE_SIZE;
	}

	void *map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED)
	{
		close(fd); // Close fd on error
		die("mmap failed.");
	}

	close(fd); // Close fd after successful mmap
	return map;
}

size_t align_up(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

// Add this helper function before main()
uint64_t get_bucket_size(size_t buffer_size)
{
	if (buffer_size < BYTES_PER_KB)
	{
		return 1; // 1ns bucket for byte-sized buffers
	}
	else if (buffer_size < BYTES_PER_MB)
	{
		return 100; // 100ns bucket for KB-sized buffers
	}
	else
	{
		return 200; // 200ns bucket for MB-sized buffers
	}
}

// Add this helper function before main()
void verify_copy_sum(void *copy_map, size_t size, size_t num_iters)
{
	volatile uint64_t *copy_pt = static_cast<volatile uint64_t *>(copy_map);
	size_t tsize = size / sizeof(uint64_t);
	uint64_t sum = 0;

	for (size_t i = 0; i < tsize; i++)
	{
		sum += copy_pt[i];
	}

	if ((sum ^ num_iters) == 0 && sum != 0)
	{
		std::cout << "Sum verification failed!" << std::endl;
		std::exit(1);
	}
}

void cache_prewarm(const void *buffer, size_t size)
{
	DEBUG_PRINT("[DEBUG] Verifying buffer sum for size: %zu bytes\n", size);

	const volatile uint64_t *buf_pt = static_cast<const volatile uint64_t *>(buffer);
	size_t num_elements = size / sizeof(uint64_t);
	uint64_t running_sum = 0;

	for (size_t i = 0; i < num_elements; i++)
	{
		running_sum += buf_pt[i];
	}

	if (running_sum == size)
	{
		std::cout << "Buffer sum verification failed!" << std::endl;
		std::cout << "Expected: " << size << ", Got: " << running_sum << std::endl;
		std::exit(1);
	}

	DEBUG_PRINT("[DEBUG] Buffer sum verified successfully: %lu\n", running_sum);
}

void flush_cache(void *ptr, size_t size)
{
	// Get alignment to 64 bytes (cache line size)
	char *p = static_cast<char *>(ptr);
	char *end = p + size;

	// Flush each cache line
	for (; p < end; p += 64)
	{
		_mm_clflushopt(p);
	}

	// Memory fence to ensure flushes complete
	_mm_sfence();
}

void run_temporal_tests(void *map, void *copy_map, size_t size, bool is_uncached,
						HistogramStats &stats_read, HistogramStats &stats_write,
						HistogramStats &stats_copy, size_t num_iters)
{
	flush_cache(map, size);

	// Skip regular reads for large uncached buffers
	if (!(is_uncached && size > 4 * BYTES_PER_KB))
	{
		DEBUG_PRINT("[DEBUG] Running regular read test (%zu iterations)...\n", num_iters);
		for (size_t i = 0; i < num_iters; i++)
		{
			run_read_benchmark(map, size, is_uncached, stats_read);
			flush_cache(map, size);
		}

		DEBUG_PRINT("[DEBUG] Running regular copy test (%zu iterations)...\n", num_iters);
		for (size_t i = 0; i < num_iters; i++)
		{
			run_copy_benchmark(map, copy_map, size, stats_copy);
			flush_cache(map, size);
		}
	}

	DEBUG_PRINT("[DEBUG] Running regular write test (%zu iterations)...\n", num_iters);
	for (size_t i = 0; i < num_iters; i++)
	{
		run_write_benchmark(map, size, is_uncached, stats_write);
		flush_cache(map, size);
	}

	DEBUG_PRINT("[DEBUG] Completed temporal tests\n");
}

void run_non_temporal_tests(void *map, void *copy_map, size_t size, bool is_uncached,
							HistogramStats &stats_read_nt, HistogramStats &stats_write_nt,
							HistogramStats &stats_copy_nt, size_t num_iters)
{
	DEBUG_PRINT("[DEBUG] Running non-temporal read test (%zu iterations)...\n", num_iters);
	for (size_t i = 0; i < num_iters; i++)
	{
		run_read_benchmark_nt(map, size, is_uncached, stats_read_nt);
	}

	DEBUG_PRINT("[DEBUG] Running non-temporal write test (%zu iterations)...\n", num_iters);
	for (size_t i = 0; i < num_iters; i++)
	{
		run_write_benchmark_nt(map, size, is_uncached, stats_write_nt);
	}

	DEBUG_PRINT("[DEBUG] Running non-temporal copy test (%zu iterations)...\n", num_iters);
	for (size_t i = 0; i < num_iters; i++)
	{
		run_copy_benchmark_nt(map, copy_map, size, stats_copy_nt);
	}

	DEBUG_PRINT("[DEBUG] Completed non-temporal tests\n");
}

void run_benchmark_iteration(void *map, void *copy_map, size_t size, bool is_uncached,
							 HistogramStats &stats_read, HistogramStats &stats_read_nt,
							 HistogramStats &stats_write, HistogramStats &stats_write_nt,
							 HistogramStats &stats_copy, HistogramStats &stats_copy_nt)
{
	DEBUG_PRINT("[DEBUG] Starting benchmark iteration for %zu bytes (%s)\n",
				size, is_uncached ? "uncached" : "cached");

	// Run non-temporal tests first
	run_non_temporal_tests(map, copy_map, size, is_uncached,
						   stats_read_nt, stats_write_nt, stats_copy_nt, NUM_ITERS);

	// Run temporal tests second
	run_temporal_tests(map, copy_map, size, is_uncached,
					   stats_read, stats_write, stats_copy, NUM_ITERS);

	DEBUG_PRINT("[DEBUG] Completed all benchmarks for iteration\n");
}

int main(int ac, char **av)
{
	// Pin process to core 3
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1)
	{
		die("Failed to set CPU affinity");
	}

	if (ac != 3)
	{
		usage(ac, av);
	}

	std::cout << "Device path: " << av[2] << std::endl; // Add this line to print device path

	int uncached_mem_test;
	if (!strcmp(av[1], "uncached"))
	{
		uncached_mem_test = 1;
	}
	else if (!strcmp(av[1], "cached"))
	{
		uncached_mem_test = 0;
	}
	else
	{
		usage(ac, av);
	}

	// Define buffer sizes to test
	std::vector<size_t> buffer_sizes = {
		64,					// 64 B
		128,				// 128 B
		BYTES_PER_KB,		// 1 KB
		4 * BYTES_PER_KB,	// 4 KB
		L1,					// L1 cache size (32 KB)
		64 * BYTES_PER_KB,	// 64 KB
		256 * BYTES_PER_KB, // 256 KB
	};

	// Run benchmarks for each buffer size
	for (size_t size : buffer_sizes)
	{
		DEBUG_PRINT("[DEBUG] ========================================\n");
		DEBUG_PRINT("[DEBUG] Starting benchmark for size: %zu bytes\n", size);
		std::cout << "\nTesting buffer size: " << size << " bytes" << std::endl;

		uint64_t bucket_size = get_bucket_size(size);
		// Histogram stats declarations
		HistogramStats stats_read("Regular read: " + std::to_string(size),
								  bucket_size, 10000, true);
		HistogramStats stats_read_nt("NT read: " + std::to_string(size),
									 bucket_size, 10000, true);
		HistogramStats stats_write("Regular write: " + std::to_string(size),
								   bucket_size, 10000, true);
		HistogramStats stats_write_nt("NT write: " + std::to_string(size),
									  bucket_size, 10000, true);
		HistogramStats stats_copy("Regular copy: " + std::to_string(size),
								  bucket_size, 10000, true);
		HistogramStats stats_copy_nt("NT copy: " + std::to_string(size),
									 bucket_size, 10000, true);

		// Allocate memory for the copy benchmarks
		void *copy_map = aligned_alloc(64, size);

		if (!copy_map)
		{
			die("Failed to allocate aligned memory for copy");
		}

		if (uncached_mem_test)
		{
			// Map a single 1MB region for all tests
			const size_t MAP_SIZE = 1024 * 1024; // 1 MB
			void *base_map = get_mem(av[2], MAP_SIZE);
			if (!base_map)
			{
				die("Failed to map memory region");
			}

			run_benchmark_iteration(base_map, copy_map, size, true,
									stats_read, stats_read_nt,
									stats_write, stats_write_nt,
									stats_copy, stats_copy_nt);

			// Check final sum
			verify_copy_sum(copy_map, size, NUM_ITERS);

			// Cleanup single mapping at the end
			munmap(base_map, MAP_SIZE);
		}
		else
		{
			// Single allocation for all tests
			void *map = aligned_alloc(64, size);
			if (!map)
			{
				die("Failed to allocate aligned memory");
			}

			run_benchmark_iteration(map, copy_map, size, false,
									stats_read, stats_read_nt,
									stats_write, stats_write_nt,
									stats_copy, stats_copy_nt);

			// Check final sum
			verify_copy_sum(copy_map, size, NUM_ITERS);

			// Free single allocation
			free(map);
		}

		free(copy_map);
		DEBUG_PRINT("[DEBUG] Completed all tests for size: %zu bytes\n\n", size);
		std::cout << std::endl;
	}

	return 0;
}
