#include "allocator.hpp"

#include <cstdlib>

namespace sandbox {

    Allocator::Allocator(const std::size_t capacity) noexcept : limit(capacity) {}

    void* Allocator::allocate(const std::size_t size) {
        if (used + size > limit) {
            throw std::bad_alloc();
        }

        void* pointer = std::malloc(size);
        if (!pointer) throw std::bad_alloc();

        used += size;
        return pointer;
    }

    void Allocator::deallocate(void* pointer, const std::size_t size) noexcept {
        if (!pointer) return;
        if (used >= size) {
            used -= size;
        } else {
            used = 0;
        }
        std::free(pointer);
    }

}