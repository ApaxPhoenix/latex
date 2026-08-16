#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include <cstdint>

namespace typesetter {

    enum class Direction : std::uint8_t {
        Horizontal,
        Vertical
    };

    [[nodiscard]] layout::Node* hbox(
        memory::Slice<layout::Node*> children,
        memory::Arena& arena
    );

    [[nodiscard]] layout::Node* vbox(
        memory::Slice<layout::Node*> children,
        memory::Arena& arena
    );

}