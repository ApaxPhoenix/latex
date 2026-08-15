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
        if (child) list.push_back(child);
    }

    const std::vector<Layer*>& Layer::children() const noexcept {
        return list;
    }

    void Layer::padding(const Edge space) noexcept {
        padding_ = space;
    }

    Layer::Edge Layer::padding() const noexcept {
        return padding_;
    }

    void Layer::margin(const Edge space) noexcept {
        margin_ = space;
    }

    Layer::Edge Layer::margin() const noexcept {
        return margin_;
    }

    Grid& Layer::grid() noexcept {
        return table;
    }

    const Grid& Layer::grid() const noexcept {
        return table;
    }

    Node::Size Layer::measure(const Node::Size boundary) noexcept {
        limit = boundary;
        const Node::Size target{
            std::max(0.0f, boundary.width - padding_.left - padding_.right - margin_.left - margin_.right),
            std::max(0.0f, boundary.height - padding_.top - padding_.bottom - margin_.top - margin_.bottom)
        };

        float span = 0.0f;
        float tall = 0.0f;
        float deep = 0.0f;

        if (item == Type::Grid) {
            span = table.measure(target).width;
            tall = table.measure(target).height;
        } else if (item == Type::Block) {
            for (auto* child : list) {
                if (!child) continue;
                span = std::max(span, child->measure(target).width);
                tall += child->measure(target).height;
            }
            for (const auto& node : elements) {
                if (node.type() == Node::Type::Box) {
                    span = std::max(span, node.box().width);
                    tall += node.box().height + node.box().depth;
                    deep = std::max(deep, node.box().depth);
                } else if (node.type() == Node::Type::Rule) {
                    span = std::max(span, node.rule().width);
                    tall += node.rule().height + node.rule().depth;
                    deep = std::max(deep, node.rule().depth);
                } else if (node.type() == Node::Type::Glue) {
                    tall += node.glue().width;
                } else if (node.type() == Node::Type::Kern) {
                    tall += node.kern().width;
                } else if (node.type() == Node::Type::Glyph) {
                    span = std::max(span, node.glyph().width);
                    tall += node.glyph().height + node.glyph().depth;
                    deep = std::max(deep, node.glyph().depth);
                }
            }
        } else {
            for (auto* child : list) {
                if (!child) continue;
                span += child->measure(target).width;
                tall = std::max(tall, child->measure(target).height);
            }
            for (const auto& node : elements) {
                if (node.type() == Node::Type::Box) {
                    span += node.box().width;
                    tall = std::max(tall, node.box().height);
                    deep = std::max(deep, node.box().depth);
                } else if (node.type() == Node::Type::Rule) {
                    span += node.rule().width;
                    tall = std::max(tall, node.rule().height);
                    deep = std::max(deep, node.rule().depth);
                } else if (node.type() == Node::Type::Glue) {
                    span += node.glue().width;
                } else if (node.type() == Node::Type::Kern) {
                    span += node.kern().width;
                } else if (node.type() == Node::Type::Glyph) {
                    span += node.glyph().width;
                    tall = std::max(tall, node.glyph().height);
                    deep = std::max(deep, node.glyph().depth);
                }
            }
        }

        return extent = {
            span + padding_.left + padding_.right + margin_.left + margin_.right,
            tall + padding_.top + padding_.bottom + margin_.top + margin_.bottom
        };
    }

    void Layer::layout(const Node::Point origin, const Node::Size size) noexcept {
        position = {origin.x + margin_.left, origin.y + margin_.top};
        extent = size;

        const float width = std::max(0.0f, size.width - padding_.left - padding_.right - margin_.left - margin_.right);
        const float height = std::max(0.0f, size.height - padding_.top - padding_.bottom - margin_.top - margin_.bottom);
        const float x = position.x + padding_.left;
        const float y = position.y + padding_.top;

        if (item == Type::Grid) {
            table.layout({x, y}, {width, height}, {list.data(), list.size()});
        } else if (item == Type::Block) {
            float offset = y;
            for (auto* child : list) {
                if (!child) continue;
                child->layout({x, offset}, {width, child->size().height});
                offset += child->size().height;
            }
        } else {
            float total = 0.0f;
            for (const auto& node : elements) {
                if (node.type() == Node::Type::Box) {
                    total += node.box().width;
                } else if (node.type() == Node::Type::Rule) {
                    total += node.rule().width;
                } else if (node.type() == Node::Type::Glue) {
                    total += node.glue().width;
                } else if (node.type() == Node::Type::Kern) {
                    total += node.kern().width;
                } else if (node.type() == Node::Type::Glyph) {
                    total += node.glyph().width;
                }
            }
            for (auto* child : list) {
                if (child) total += child->size().width;
            }

            const float delta = width - total;
            std::uint8_t rank = 0;
            float flex = 0.0f;

            if (delta != 0.0f) {
                for (const auto& node : elements) {
                    if (node.type() == Node::Type::Glue) {
                        const float stretch = (delta > 0.0f) ? node.glue().stretch : node.glue().shrink;
                        const auto order = static_cast<std::uint8_t>(node.glue().stretchorder);
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
                if (node.type() == Node::Type::Box) {
                    cursor += node.box().width;
                } else if (node.type() == Node::Type::Rule) {
                    cursor += node.rule().width;
                } else if (node.type() == Node::Type::Glue) {
                    float advance = node.glue().width;
                    if (static_cast<std::uint8_t>(node.glue().stretchorder) == rank) {
                        advance += ((delta > 0.0f) ? node.glue().stretch : node.glue().shrink) * scale;
                    }
                    cursor += std::max(0.0f, advance);
                } else if (node.type() == Node::Type::Kern) {
                    cursor += node.kern().width;
                } else if (node.type() == Node::Type::Glyph) {
                    cursor += node.glyph().width;
                }
            }

            for (auto* child : list) {
                if (!child) continue;
                child->layout({cursor, y}, {child->size().width, height});
                cursor += child->size().width;
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