#pragma once

#include "memory/arena.hpp"

#include <cstdint>

namespace render::typography {

    class Protrusion {
    public:
        struct Margin {
            std::int32_t left{0};
            std::int32_t right{0};
        };

        struct Node {
            std::uint32_t code{0};
            Margin margin{};
            Node* next{nullptr};
        };

        explicit Protrusion(memory::Arena& arena, std::size_t slots = 256) noexcept;

        void compose(std::uint32_t code, std::int32_t left, std::int32_t right) const noexcept;
        [[nodiscard]] Margin get(std::uint32_t code) const noexcept;

    private:
        memory::Arena& arena;
        std::size_t slots{0};
        Node** table{nullptr};
    };

}