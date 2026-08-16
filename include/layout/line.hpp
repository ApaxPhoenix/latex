#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

namespace layout {

    class Line {
    public:
        static Node* horizontal(memory::Arena& arena, memory::Slice<Node*> nodes, float target) noexcept;
        static Node* vertical(memory::Arena& arena, memory::Slice<Node*> nodes, float skip) noexcept;
    };

}