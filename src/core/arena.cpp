#include "core/arena.hpp"

namespace core::arena {

    Arena::Arena(const size_t size) : capacity(size) {
        this->grow();
    }

    Arena::~Arena() {
        this->dispose();
    }

    void* Arena::allocate(const size_t size, const size_t alignment) {
        if (size > this->capacity) {
            throw std::bad_alloc();
        }

        auto* chunk = &this->chunks.back();
        size_t offset = chunk->offset + alignment - 1 & ~(alignment - 1);

        if (offset + size > chunk->capacity) {
            this->grow();
            chunk = &this->chunks.back();
            offset = 0;
        }

        void* memory = chunk->buffer + offset;
        chunk->offset = offset + size;

        return memory;
    }

    void Arena::grow() {
        auto* buffer = static_cast<uint8_t*>(std::malloc(capacity));

        if (buffer == nullptr) {
            throw std::bad_alloc();
        }

        try {
            this->chunks.push_back(Chunk{buffer, this->capacity, 0});
        } catch (...) {
            std::free(buffer);
            throw;
        }
    }

    void Arena::dispose() {
        for (const auto& chunk : this->chunks) {
            std::free(chunk.buffer);
        }

        this->chunks.clear();
    }
}