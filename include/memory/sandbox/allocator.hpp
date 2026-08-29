#pragma once

#include <cstddef>
#include <new>

namespace sandbox {

    class Allocator {
    public:
        explicit Allocator(std::size_t capacity) noexcept;
        ~Allocator() = default;

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator(Allocator&&) noexcept = delete;
        Allocator& operator=(Allocator&&) noexcept = delete;

        [[nodiscard]] void* allocate(std::size_t size);
        void deallocate(void* ptr, std::size_t size) noexcept;

        [[nodiscard]] std::size_t allocated() const noexcept { return used; }
        [[nodiscard]] std::size_t capacity() const noexcept { return limit; }

    private:
        std::size_t limit{0};
        std::size_t used{0};
    };

}