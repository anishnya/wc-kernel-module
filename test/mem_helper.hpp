#ifndef WC_MEM_HELPER_HPP
#define WC_MEM_HELPER_HPP

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "constants.h"  // for PAGE_SIZE, PAGE_MASK, CACHE_LINE_SIZE, CACHE_LINE_MASK

std::size_t align_up_size(std::size_t size, std::size_t alignment)
{
    std::size_t mask = alignment - 1;
    std::size_t aligned = (size + mask) & ~mask;
    DEBUG_PRINT("Aligning size " << size << " to " << aligned << " (alignment: " << alignment
                                 << ")");
    return aligned;
}

void *cache_align(void *ptr)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t aligned = (addr + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
    DEBUG_PRINT("Cache aligning address 0x" << std::hex << addr << " to 0x" << aligned << std::dec);
    return reinterpret_cast<void *>(aligned);
}

// Must be Page aligned due to implementation of wc_ram
void *get_wc_mem(char *dev, int size)
{
    DEBUG_PRINT("Allocating " << size << " bytes of write-combining memory from " << dev);

    int fd = open(dev, O_RDWR, 0);
    if (fd == -1)
    {
        DEBUG_PRINT("Failed to open device " << dev);
        close(fd);
        std::exit(1);
    }

    void *map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        DEBUG_PRINT("Failed to mmap " << size << " bytes from device " << dev);
        close(fd);
        std::exit(1);
    }

    DEBUG_PRINT("Successfully mapped WC memory at address 0x"
                << std::hex << reinterpret_cast<uintptr_t>(map) << std::dec);
    close(fd);
    return map;
}

void *get_wb_mem(int size)
{
    DEBUG_PRINT("Allocating " << size << " bytes of write-back memory");

    void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        DEBUG_PRINT("Failed to mmap " << size << " bytes of write-back memory");
        std::exit(1);
    }

    DEBUG_PRINT("Successfully mapped WB memory at address 0x"
                << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec);
    return ptr;
}

#endif  // WC_MEM_HELPER_HPP