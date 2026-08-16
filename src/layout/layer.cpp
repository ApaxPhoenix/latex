#include "layout/layer.hpp"
#include <algorithm>

namespace layout {

    Layer::Layer(const Type value) noexcept
        : type_(value) {}

    void Layer::type(const Type value) noexcept { type_ = value; }
    Layer::Type Layer::type() const noexcept { return type_; }

    void Layer::nodes(const memory::Slice<Node> slice) noexcept { elements = slice; }
    memory::Slice<Node> Layer::nodes() const noexcept { return elements; }

    void Layer::attach(Layer* child) noexcept {
        if (child) list.push_back(child);
    }

    const std::vector<Layer*>& Layer::children() const noexcept { return list; }

    void Layer::padding(Edge space) noexcept { padding_ = space; }
    Layer::Edge Layer::padding() const noexcept { return padding_; }

    void Layer::margin(Edge space) noexcept { margin_ = space; }
    Layer::Edge Layer::margin() const noexcept { return margin_; }

    Grid& Layer::grid() noexcept { return table; }
    const Grid& Layer::grid() const noexcept { return table; }

    Node::Point Layer::origin() const noexcept { return position; }
    Node::Size Layer::size() const noexcept { return extent; }
    Node::Size Layer::bounds() const noexcept { return limit; }

    Node::Size Layer::measure(const Node::Size boundary) noexcept {
        float width = boundary.width - (padding_.left + padding_.right + margin_.left + margin_.right);
        float height = boundary.height - (padding_.top + padding_.bottom + margin_.top + margin_.bottom);
        width = std::max(0.0f, width);
        height = std::max(0.0f, height);

        Node::Size size{0.0f, 0.0f};

        if (type_ == Type::Grid) {
            size = table.measure(Node::Size{width, height});
        } else {
            for (Layer* child : list) {
                if (!child) continue;
                const Node::Size bounds = child->measure(Node::Size{width, height});
                if (type_ == Type::Block) {
                    size.width = std::max(size.width, bounds.width);
                    size.height += bounds.height;
                } else if (type_ == Type::Inline) {
                    size.width += bounds.width;
                    size.height = std::max(size.height, bounds.height);
                }
            }
        }

        extent = Node::Size{
            .width = size.width + padding_.left + padding_.right,
            .height = size.height + padding_.top + padding_.bottom
        };

        limit = Node::Size{
            .width = extent.width + margin_.left + margin_.right,
            .height = extent.height + margin_.top + margin_.bottom
        };

        return limit;
    }

    void Layer::layout(const Node::Point origin, const Node::Size size) noexcept {
        position = Node::Point{
            origin.x + margin_.left,
            origin.y + margin_.top
        };

        extent = Node::Size{
            std::max(0.0f, size.width - (margin_.left + margin_.right)),
            std::max(0.0f, size.height - (margin_.top + margin_.bottom))
        };

        limit = size;

        const Node::Point location{
            position.x + padding_.left,
            position.y + padding_.top
        };

        const Node::Size bounds{
            std::max(0.0f, extent.width - (padding_.left + padding_.right)),
            std::max(0.0f, extent.height - (padding_.top + padding_.bottom))
        };

        if (type_ == Type::Grid) {
            const memory::Slice children((list.data()), list.size());
            table.layout(location, bounds, children);
        } else {
            Node::Point cursor = location;

            for (Layer* child : list) {
                if (!child) continue;
                const Node::Size box = child->measure(bounds);

                child->layout(cursor, box);

                if (type_ == Type::Block) {
                    cursor.y += box.height;
                } else if (type_ == Type::Inline) {
                    cursor.x += box.width;
                }
            }
        }
    }

}