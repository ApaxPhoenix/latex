#include "typesetter/text.hpp"

namespace typesetter {

    memory::Slice<layout::Node*> text(
        const std::string_view source,
        const typography::Font& font,
        const typography::Shaper& shaper,
        const typography::Hyphenator& hyphenator,
        const memory::Arena& arena
    ) {
        std::ignore = hyphenator;
        std::ignore = arena;
        return shaper.shape(font, source);
    }

}