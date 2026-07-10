#include "arena.hpp"
#include <algorithm>
#include <cassert>
#include <new>

#include "arena.hpp"

namespace core::arena {

    // Constructor
    Arena::Arena(const size_t size) : capacity(size) {
        // 1. Call your internal block allocator helper to boot up the first chunk
        this->grow();
    }

    // Destructor
    Arena::~Arena() {
        // 1. Call your dispose method to ensure all OS allocations are freed
        this->dispose();
    }

    // Allocation Logic
    void* Arena::allocate(size_t size, size_t alignment) {
        
    }

    // Memory Cleanup
    void Arena::dispose() {

    }

    // Allocation Internal Helper (grow)
    void Arena::grow() {

    }

}