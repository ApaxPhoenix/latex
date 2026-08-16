#pragma once

#include "expression/node.hpp"
#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "typography/font.hpp"

namespace typesetter {

    [[nodiscard]] layout::Node* equations(
        const expression::Node& node,
        const typography::Font& font,
        memory::Arena& arena
    );

}