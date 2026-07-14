#pragma once

#include <vector>
#include <memory>
#include <new>
#include <utility>

namespace core::arena {
    class Arena {
    public:
        explicit Arena(size_t size = 4 * 1024 * 1024);
        ~Arena();

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;

        Arena(Arena&&) noexcept = default;
        Arena& operator=(Arena&&) noexcept = default;

        void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));

        template <typename T, typename... Arguments>
        T* construct(Arguments&&... arguments) {
            void* memory = allocate(sizeof(T), alignof(T));
            return ::new (memory) T(std::forward<Arguments>(arguments)...);
        }

        void dispose();

    private:
        struct Chunk {
            uint8_t* buffer;
            size_t capacity;
            size_t offset;
        };

        std::vector<Chunk> chunks;
        size_t capacity;

        void grow();
    };
}