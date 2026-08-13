#include "layout/box.hpp"

namespace layout {

    void Box::padding(const Layer::Edge inset) noexcept {
        padding_ = inset;
    }

    void Box::margin(const Layer::Edge inset) noexcept {
        margin_ = inset;
    }

    void Box::alignment(const Type alignment) noexcept {
        alignment_ = alignment;
    }

    Node::Size Box::measure(Layer* child, const Node::Size boundary) const noexcept {
        if (!child) {
            return Node::Size{0.0f, 0.0f, 0.0f};
        }

        const Node::Size limit{
            boundary.width - padding_.left - padding_.right - margin_.left - margin_.right,
            boundary.height - padding_.top - padding_.bottom - margin_.top - margin_.bottom,
            0.0f
        };

        const auto [width, height, depth] = child->measure(limit);

        return Node::Size{
            width + padding_.left + padding_.right + margin_.left + margin_.right,
            height + padding_.top + padding_.bottom + margin_.top + margin_.bottom,
            depth
        };
    }

    void Box::layout(Layer* child, const Node::Point origin, const Node::Size size) const noexcept {
        if (!child) {
            return;
        }

        const Node::Point pos{
            origin.x + margin_.left + padding_.left,
            origin.y + margin_.top + padding_.top
        };

        const Node::Size extent{
            size.width - padding_.left - padding_.right - margin_.left - margin_.right,
            size.height - padding_.top - padding_.bottom - margin_.top - margin_.bottom,
            size.depth
        };

        child->layout(pos, extent);
    }

}