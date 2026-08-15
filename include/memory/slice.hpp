#pragma once

#include <cstddef>

namespace memory {

    template <typename Type>
    struct Slice {
        Type* data = nullptr;
        std::size_t count = 0;

        [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }
        [[nodiscard]] constexpr std::size_t size() const noexcept { return count; }
        constexpr Type* begin() noexcept { return data; }
        constexpr Type* end() noexcept { return data + count; }
        constexpr const Type* begin() const noexcept { return data; }
        constexpr const Type* end() const noexcept { return data + count; }
        constexpr Type& operator[](std::size_t index) noexcept { return data[index]; }
        constexpr const Type& operator[](std::size_t index) const noexcept { return data[index]; }
    };

}