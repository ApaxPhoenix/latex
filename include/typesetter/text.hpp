#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "typography/font.hpp"
#include "typography/hyphenator.hpp"
#include "typography/shaper.hpp"
#include <string_view>

namespace typesetter {

    memory::Slice<layout::Node*> text(
        std::string_view source,
        const typography::Font& font,
        const typography::Shaper& shaper,
        const typography::Hyphenator& hyphenator,
        const memory::Arena& arena
    );

}