#include "memory/arena.hpp"

#include <cstring>
#include <memory>

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

        auto address = reinterpret_cast<std::uintptr_t>(block->buffer.get() + block->offset);
        std::uintptr_t aligned = address + alignment - 1 & ~(alignment - 1);
        std::size_t padding = aligned - address;

        if (block->offset + padding + size > block->capacity) {
            grow(size + alignment);
            block = &blocks.back();

            address = reinterpret_cast<std::uintptr_t>(block->buffer.get());
            aligned = (address + alignment - 1) & ~(alignment - 1);
            padding = aligned - address;
        }

        std::uint8_t* destination = block->buffer.get() + block->offset + padding;
        block->offset += padding + size;

        return destination;
    }

    std::string_view Arena::copy(const std::string_view input) {
        if (input.empty()) return {};

        return std::string_view(
            static_cast<const char*>(std::memcpy(allocate(input.size(), alignof(char)), input.data(), input.size())),
            input.size()
        );
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