#include "typesetter/equations.hpp"

namespace typesetter {

    layout::Node* equations(
        const expression::Node& node,
        const typography::Font& font,
        memory::Arena& arena
    ) {
        std::ignore = node;
        std::ignore = font;

        auto* head = arena.compose<layout::Node>(layout::Node::Type::Box);
        return head;
    }

}