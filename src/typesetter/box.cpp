#include "typesetter/box.hpp"

namespace typesetter {

    layout::Node* hbox(
        const memory::Slice<layout::Node*> children,
        memory::Arena& arena
    ) {
        auto* node = arena.compose<layout::Node>(layout::Node::Type::Box);
        node->box().list = children;
        return node;
    }

    layout::Node* vbox(
        const memory::Slice<layout::Node*> children,
        memory::Arena& arena
    ) {
        auto* node = arena.compose<layout::Node>(layout::Node::Type::Box);
        node->box().list = children;
        return node;
    }

}