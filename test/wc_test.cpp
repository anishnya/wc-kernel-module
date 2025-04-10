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
#include "constants.h"
#include "stats.hpp"
#include "bench.hpp"

void usage(int ac, char **av)
{
	std::cout << "usage: " << av[0] << " cached   mem_dev\n";
	std::cout << "       " << av[0] << " uncached mem_dev\n";
	exit(1);
}

void die(char *msg)
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

	std::cout << "mmap()'ing " << dev << std::endl;

	if (size & ~PAGE_MASK)
	{
		size = (size & PAGE_MASK) + PAGE_SIZE;
	}

	void *map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED)
	{
		die("mmap failed.");
	}
	return map;
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

int main(int ac, char **av)
{
	if (ac != 3)
	{
		usage(ac, av);
	}

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
		64,				   // 64 B
		128,			   // 128 B
		BYTES_PER_KB,	   // 1 KB
		4 * BYTES_PER_KB,  // 4 KB
		L1,				   // L1 cache size (32 KB)
		64 * BYTES_PER_KB, // 64 KB
		L2,				   // L2 cache size (1 MB)
		4 * BYTES_PER_MB,  // 4 MB
		L3,				   // L3 cache size (40 MB)
		16 * BYTES_PER_MB, // 16 MB
		1 * BYTES_PER_GB,  // 2 GB
	};

	// Run benchmarks for each buffer size
	for (size_t size : buffer_sizes)
	{
		std::cout << "\nTesting buffer size: " << size << " bytes" << std::endl;

		if (uncached_mem_test)
		{
			uint64_t bucket_size = get_bucket_size(size);
			// For uncached memory, run both regular and non-temporal tests
			HistogramStats stats_read("Uncached regular read: " + std::to_string(size),
									  bucket_size, 1000, true);
			HistogramStats stats_read_nt("Uncached NT read: " + std::to_string(size),
										 bucket_size, 1000, true);
			HistogramStats stats_write("Uncached regular write: " + std::to_string(size),
									   bucket_size, 1000, true);
			HistogramStats stats_write_nt("Uncached NT write: " + std::to_string(size),
										  bucket_size, 1000, true);

			for (int iter = 0; iter < NUM_ITERS; iter++)
			{
				// Regular read test
				void *map = get_mem(av[2], size);
				run_read_benchmark(map, size, true, stats_read);
				munmap(map, size);

				// Non-temporal read test
				map = get_mem(av[2], size);
				run_read_benchmark_nt(map, size, true, stats_read_nt);
				munmap(map, size);

				// Regular write test
				map = get_mem(av[2], size);
				run_write_benchmark(map, size, true, stats_write);
				munmap(map, size);

				// Non-temporal write test
				map = get_mem(av[2], size);
				run_write_benchmark_nt(map, size, true, stats_write_nt);
				munmap(map, size);
			}
		}
		else
		{
			uint64_t bucket_size = get_bucket_size(size);
			// For cached memory, run both regular and non-temporal tests
			HistogramStats stats_read("Cached regular read: " + std::to_string(size),
									  bucket_size, 1000, true);
			HistogramStats stats_read_nt("Cached NT read: " + std::to_string(size),
										 bucket_size, 1000, true);
			HistogramStats stats_write("Cached regular write: " + std::to_string(size),
									   bucket_size, 1000, true);
			HistogramStats stats_write_nt("Cached NT write: " + std::to_string(size),
										  bucket_size, 1000, true);

			for (int iter = 0; iter < NUM_ITERS; iter++)
			{
				// Regular read test
				void *map = malloc(size);
				run_read_benchmark(map, size, false, stats_read);
				free(map);

				// Non-temporal read test
				map = malloc(size);
				run_read_benchmark_nt(map, size, false, stats_read_nt);
				free(map);

				// Regular write test
				map = malloc(size);
				run_write_benchmark(map, size, false, stats_write);
				free(map);

				// Non-temporal write test
				map = malloc(size);
				run_write_benchmark_nt(map, size, false, stats_write_nt);
				free(map);
			}
		}

		std::cout << std::endl;
	}

	return 0;
}
