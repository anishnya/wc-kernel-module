#ifndef WC_MEM_HPP
#define WC_MEM_HPP

#include <cstdint>

#include "mem_helper.hpp"

enum class MemoryType : std::uint8_t
{
    WB = 0,  // Regular cached
    WC = 1,  // Write-combining
    UC = 2   // Uncached
};

enum class CacheFlushType : std::uint8_t
{
    NO_FLUSH = 0,
    FLUSH = 1,
};

struct MemArena
{
    void* base_ptr;
    void* ptr;  // Cache aligned pointer
    std::size_t size;
    MemoryType type;
    CacheFlushType flush_type;

    MemArena(const char* dev, std::size_t size, MemoryType memory_type, CacheFlushType flush_type)
        : type(memory_type), flush_type(flush_type)
    {
        this->size = align_up_size(size + CACHE_LINE_SIZE, PAGE_SIZE);
        if (type == MemoryType::WC)
        {
            base_ptr = get_wc_mem(const_cast<char*>(dev), this->size);
        }
        else if (type == MemoryType::WB)
        {
            base_ptr = get_wb_mem(this->size);
        }
        else
        {
            std::exit(1);
        }

        this->ptr = cache_align(base_ptr);
    }

    ~MemArena() { munmap(base_ptr, size); }
};

#endif  // WC_MEM_HPP