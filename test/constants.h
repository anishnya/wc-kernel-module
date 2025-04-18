#ifndef WC_CONSTANTS_H
#define WC_CONSTANTS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>

// Debug print macro
#ifdef WC_DEBUG
#define DEBUG_PRINT(msg, ...)                        \
    do                                               \
    {                                                \
        std::cout << "[DEBUG] " << msg << std::endl; \
    } while (0)
#else
#define DEBUG_PRINT(msg, ...) \
    do                        \
    {                         \
    } while (0)
#endif

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define CACHE_LINE_SIZE 64
#define CACHE_LINE_MASK (~(CACHE_LINE_SIZE - 1))

// Size constants (in bytes)
constexpr const std::size_t BYTES_PER_KB = 1024;
constexpr const std::size_t BYTES_PER_MB = 1024 * BYTES_PER_KB;
constexpr const std::size_t BYTES_PER_GB = 1024 * BYTES_PER_MB;

// Cache boundary sizes
constexpr const std::size_t L1 = 32 * BYTES_PER_KB;
constexpr const std::size_t L2 = 1 * BYTES_PER_MB;
constexpr const std::size_t L3 = 32 * BYTES_PER_MB;

// Test parameters
constexpr const std::size_t NUM_ITERS = 1000000;
constexpr const std::uint64_t TEST_PATTERN = 0xDEADBEEF;
constexpr const std::uint64_t HISTO_SIZE = 100000;

constexpr std::uint8_t ALL_EQUAL_MASK = 0xFF;
constexpr std::size_t UNROLL_FACTOR = 8;

constexpr std::array<const std::size_t, 12> BUFFER_SIZES = {
    256 * BYTES_PER_KB,   // 256 KB
    512 * BYTES_PER_KB,   // 512 KB
    768 * BYTES_PER_KB,   // 768 KB
    1024 * BYTES_PER_KB,  // 1024 KB (1 MB)
    1280 * BYTES_PER_KB,  // 1280 KB (1.25 MB)
    1536 * BYTES_PER_KB,  // 1536 KB (1.5 MB)
    1792 * BYTES_PER_KB,  // 1792 KB (1.75 MB)
    2048 * BYTES_PER_KB,  // 2048 KB (2 MB)
    2304 * BYTES_PER_KB,  // 2304 KB (2.25 MB)
    2560 * BYTES_PER_KB,  // 2560 KB (2.5 MB)
    2816 * BYTES_PER_KB,  // 2816 KB (2.75 MB)
    3072 * BYTES_PER_KB   // 3072 KB (3 MB)
};

constexpr std::array<const std::size_t, 1> BATCH_SIZES = {1};

#endif  // WC_CONSTANTS_H