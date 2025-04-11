#ifndef WC_CONSTANTS_H
#define WC_CONSTANTS_H

#include <cstddef> // for size_t

// Size constants (in bytes)
constexpr size_t BYTES_PER_KB = 1024;
constexpr size_t BYTES_PER_MB = 1024 * BYTES_PER_KB;
constexpr size_t BYTES_PER_GB = 1024 * BYTES_PER_MB;

// Cache boundary sizes
constexpr size_t L1 = 32 * BYTES_PER_KB;
constexpr size_t L2 = 1 * BYTES_PER_MB;
constexpr size_t L3 = 32 * BYTES_PER_MB;

constexpr size_t NUM_ITERS = 1000000;

#endif // WC_CONSTANTS_H