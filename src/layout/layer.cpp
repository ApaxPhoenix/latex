#include "layout/layer.hpp"
#include <algorithm>

namespace layout {

Layer::Layer(const Type value) noexcept : item(value) {}

void Layer::type(const Type value) noexcept {
    item = value;
}

Layer::Type Layer::type() const noexcept {
    return item;
}

void Layer::nodes(const memory::Slice<Node> slice) noexcept {
    elements = slice;
}

memory::Slice<Node> Layer::nodes() const noexcept {
    return elements;
}

void Layer::attach(Layer* child) noexcept {
    if (child) {
        list.push_back(child);
    }
}

const std::vector<Layer*>& Layer::children() const noexcept {
    return list;
}

void Layer::padding(const Edge space) noexcept {
    pad = space;
}

Layer::Edge Layer::padding() const noexcept {
    return pad;
}

void Layer::margin(const Edge space) noexcept {
    gap = space;
}

Layer::Edge Layer::margin() const noexcept {
    return gap;
}

Grid& Layer::grid() noexcept {
    return table;
}

const Grid& Layer::grid() const noexcept {
    return table;
}

Node::Size Layer::measure(const Node::Size boundary) noexcept {
    limit = boundary;

    const float width = std::max(0.0f, boundary.width - pad.left - pad.right - gap.left - gap.right);
    const float height = std::max(0.0f, boundary.height - pad.top - pad.bottom - gap.top - gap.bottom);
    const Node::Size target{width, height, boundary.depth};

    float span = 0.0f;
    float tall = 0.0f;
    float deep = 0.0f;

    if (item == Type::Grid) {
        const auto size = table.measure(target);
        span = size.width;
        tall = size.height;
        deep = size.depth;
    } else if (item == Type::Block) {
        for (auto* child : list) {
            if (!child) continue;
            const auto [w, h, d] = child->measure(target);
            span = std::max(span, w);
            tall += h;
            deep = std::max(deep, d);
        }

        for (const auto& node : elements) {
            switch (node.type()) {
                case Node::Type::box: {
                    const auto [w, h, d] = node.box().size;
                    span = std::max(span, w);
                    tall += h;
                    deep = std::max(deep, d);
                    break;
                }
                case Node::Type::rule: {
                    const auto& rule = node.rule();
                    span = std::max(span, rule.width);
                    tall += rule.height;
                    deep = std::max(deep, rule.depth);
                    break;
                }
                case Node::Type::glue: {
                    tall += node.glue().width;
                    break;
                }
                case Node::Type::kern: {
                    tall += node.kern().width;
                    break;
                }
                case Node::Type::penalty:
                    break;
            }
        }
    } else {
        for (auto* child : list) {
            if (!child) continue;
            const auto [w, h, d] = child->measure(target);
            span += w;
            tall = std::max(tall, h);
            deep = std::max(deep, d);
        }

        for (const auto& node : elements) {
            switch (node.type()) {
                case Node::Type::box: {
                    const auto [w, h, d] = node.box().size;
                    span += w;
                    tall = std::max(tall, h);
                    deep = std::max(deep, d);
                    break;
                }
                case Node::Type::rule: {
                    const auto& rule = node.rule();
                    span += rule.width;
                    tall = std::max(tall, rule.height);
                    deep = std::max(deep, rule.depth);
                    break;
                }
                case Node::Type::glue: {
                    span += node.glue().width;
                    break;
                }
                case Node::Type::kern: {
                    span += node.kern().width;
                    break;
                }
                case Node::Type::penalty:
                    break;
            }
        }
    }

    extent.width = span + pad.left + pad.right + gap.left + gap.right;
    extent.height = tall + pad.top + pad.bottom + gap.top + gap.bottom;
    extent.depth = deep;

    return extent;
}

void Layer::layout(const Node::Point origin, const Node::Size size) noexcept {
    position = Node::Point{origin.x + gap.left, origin.y + gap.top};
    extent = size;

    const float width = std::max(0.0f, size.width - pad.left - pad.right - gap.left - gap.right);
    const float height = std::max(0.0f, size.height - pad.top - pad.bottom - gap.top - gap.bottom);

    const float x = position.x + pad.left;
    const float y = position.y + pad.top;

    if (item == Type::Grid) {
        table.layout(Node::Point{x, y}, Node::Size{width, height, size.depth}, list);
    } else if (item == Type::Block) {
        float offset = y;

        for (auto* child : list) {
            if (!child) continue;
            const auto [w, h, d] = child->size();
            child->layout(Node::Point{x, offset}, Node::Size{width, h, d});
            offset += h;
        }
    } else {
        float total = 0.0f;

        for (const auto& node : elements) {
            switch (node.type()) {
                case Node::Type::box:   total += node.box().size.width; break;
                case Node::Type::rule:  total += node.rule().width; break;
                case Node::Type::glue:  total += node.glue().width; break;
                case Node::Type::kern:  total += node.kern().width; break;
                case Node::Type::penalty: break;
            }
        }

        for (auto* child : list) {
            if (child) {
                total += child->size().width;
            }
        }

        const float delta = width - total;

        std::uint8_t rank = 0;
        float flex = 0.0f;

        if (delta != 0.0f) {
            for (const auto& node : elements) {
                if (node.type() == Node::Type::glue) {
                    const auto& glue = node.glue();
                    const float stretch = (delta > 0.0f) ? glue.stretch : glue.shrink;
                    const auto order = static_cast<std::uint8_t>(glue.order);
                    if (stretch > 0.0f) {
                        if (order > rank) {
                            rank = order;
                            flex = stretch;
                        } else if (order == rank) {
                            flex += stretch;
                        }
                    }
                }
            }
        }

        const float scale = (flex > 0.0f) ? (delta / flex) : 0.0f;

        float cursor = x;

        for (auto& node : elements) {
            switch (node.type()) {
                case Node::Type::box: {
                    auto& box = node.box();
                    box.point = Node::Point{cursor, y};
                    cursor += box.size.width;
                    break;
                }
                case Node::Type::rule: {
                    cursor += node.rule().width;
                    break;
                }
                case Node::Type::glue: {
                    const auto& [w, stretch, shrink, order] = node.glue();
                    float advance = w;
                    if (static_cast<std::uint8_t>(order) == rank) {
                        const float extra = (delta > 0.0f) ? stretch : shrink;
                        advance += extra * scale;
                    }
                    cursor += std::max(0.0f, advance);
                    break;
                }
                case Node::Type::kern: {
                    cursor += node.kern().width;
                    break;
                }
                case Node::Type::penalty:
                    break;
            }
        }

        for (auto* child : list) {
            if (!child) continue;
            const auto [w, h, d] = child->size();
            child->layout(Node::Point{cursor, y}, Node::Size{w, height, d});
            cursor += w;
        }
    }
}

Node::Point Layer::origin() const noexcept {
    return position;
}

Node::Size Layer::size() const noexcept {
    return extent;
}

Node::Size Layer::bounds() const noexcept {
    return limit;
}

}