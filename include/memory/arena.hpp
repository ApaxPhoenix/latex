#pragma once

#include "memory/slice.hpp"

#include <cstdint>
#include <memory>
#include <new>
#include <string_view>
#include <utility>
#include <vector>

namespace memory {

    class Arena {
    public:
        struct Block {
            std::unique_ptr<std::uint8_t[]> buffer;
            std::size_t capacity = 0;
            std::size_t offset = 0;
        };

        explicit Arena(std::size_t capacity = 65536);
        ~Arena();

        Arena(const Arena&) = delete;
        Arena& operator=(const Arena&) = delete;
        Arena(Arena&&) noexcept = default;
        Arena& operator=(Arena&&) noexcept = default;

        [[nodiscard]] void* allocate(std::size_t size, std::size_t alignment);

        template <typename Type>
        [[nodiscard]] Slice<Type> allocate(std::size_t count) {
            if (!count) return {};
            return Slice<Type>{static_cast<Type*>(allocate(sizeof(Type) * count, alignof(Type))), count};
        }

        template <typename Type, typename... Arguments>
        [[nodiscard]] Type* compose(Arguments&&... arguments) {
            void* pointer = allocate(sizeof(Type), alignof(Type));
            return ::new (pointer) Type(std::forward<Arguments>(arguments)...);
        }

        [[nodiscard]] std::string_view copy(std::string_view input);

        void reset() noexcept;
        void dispose() noexcept;

    private:
        void grow(std::size_t minimum = 0);

        std::size_t capacity = 0;
        std::vector<Block> blocks;
    };

}