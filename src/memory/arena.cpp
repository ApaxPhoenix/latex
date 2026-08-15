#include "memory/arena.hpp"

#include <cstring>

namespace memory {

    Arena::Arena(const std::size_t capacity) : capacity(capacity) {
        grow(capacity);
    }

    Arena::~Arena() {
        dispose();
    }

    void* Arena::allocate(const std::size_t size, const std::size_t alignment) {
        if (!size) return nullptr;

        if (blocks.empty()) grow(size);

        auto* block = &blocks.back();
        std::size_t offset = (block->offset + alignment - 1) & ~(static_cast<std::size_t>(alignment) - 1);

        if (offset + size > block->capacity) {
            grow(size);
            block = &blocks.back();
            offset = (block->offset + alignment - 1) & ~(static_cast<std::size_t>(alignment) - 1);
        }

        void* pointer = block->buffer.get() + offset;
        block->offset = offset + size;
        return pointer;
    }

    std::string_view Arena::copy(const std::string_view input) {
        if (input.empty()) return {};

        auto* pointer = static_cast<char*>(allocate(input.size(), alignof(char)));
        std::memcpy(pointer, input.data(), input.size());
        return std::string_view(pointer, input.size());
    }

    void Arena::grow(const std::size_t minimum) {
        std::size_t target = capacity ? capacity : 65536;
        while (target < minimum) target *= 2;

        blocks.emplace_back(std::make_unique<std::uint8_t[]>(target), target, 0);
        capacity = target * 2;
    }

    void Arena::reset() noexcept {
        for (auto& block : blocks) block.offset = 0;
    }

    void Arena::dispose() noexcept {
        blocks.clear();
        capacity = 0;
    }

}