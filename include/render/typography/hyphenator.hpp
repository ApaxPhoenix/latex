#pragma once

#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <cstdint>
#include <string_view>

namespace render::typography {

    class Hyphenator {
    public:
        struct Node {
            std::uint32_t code{0};
            Node* child{nullptr};
            Node* next{nullptr};
            memory::Slice<std::uint8_t> levels{};
        };

        explicit Hyphenator(memory::Arena& arena) noexcept;

        void load(std::string_view path) const;
        void compose(memory::Slice<std::uint32_t> pattern, memory::Slice<std::uint8_t> levels) const noexcept;
        [[nodiscard]] memory::Slice<std::uint8_t> execute(
            memory::Arena& scratch,
            memory::Slice<std::uint32_t> word,
            std::uint32_t pad = '.',
            std::size_t boundary = 2
        ) const;

    private:
        memory::Arena& arena;
        Node* root{nullptr};
    };

}