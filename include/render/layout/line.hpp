#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

namespace render::layout {

    class Line {
    public:
        [[nodiscard]] static Node* horizontal(
            memory::Arena& arena,
            memory::Slice<Node*> nodes,
            float target = 0.0f
        ) noexcept;

        [[nodiscard]] static Node* vertical(
            memory::Arena& arena,
            memory::Slice<Node*> nodes,
            float skip = 0.0f
        ) noexcept;
    };

}