#include "typesetter/paragraph.hpp"
#include "typesetter/text.hpp"

namespace typesetter {

    layout::Node* paragraph(
        const std::string_view source,
        const typography::Font& font,
        const typography::Shaper& shaper,
        const typography::Hyphenator& hyphenator,
        layout::Breaker& breaker,
        memory::Arena& arena
    ) {
        const auto stream = text(source, font, shaper, hyphenator, arena);
        const auto lines = breaker.compose(stream);
        auto* node = arena.compose<layout::Node>(layout::Node::Type::Box);
        node->box().list = lines;
        return node;
    }

}