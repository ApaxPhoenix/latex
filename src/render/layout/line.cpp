#include "layout/line.hpp"

#include <algorithm>
#include <array>

namespace render::layout {

    Node* Line::horizontal(
        memory::Arena& arena,
        memory::Slice<Node*> nodes,
        const float target
    ) noexcept {
        auto* box = arena.compose<Node>(Node::Type::Box);

        float natural = 0.0f;
        std::array stretch{0.0f, 0.0f, 0.0f, 0.0f};
        std::array shrink{0.0f, 0.0f, 0.0f, 0.0f};

        float height = 0.0f;
        float depth = 0.0f;

        for (const auto* node : nodes) {
            if (!node) continue;

            switch (node->type()) {
                case Node::Type::Glyph: {
                    const auto& glyph = node->glyph();
                    natural += glyph.width;
                    height = std::max(height, glyph.height);
                    depth = std::max(depth, glyph.depth);
                    break;
                }
                case Node::Type::Box: {
                    const auto& inner = node->box();
                    natural += inner.width;
                    height = std::max(height, inner.height);
                    depth = std::max(depth, inner.depth);
                    break;
                }
                case Node::Type::Glue: {
                    const auto& glue = node->glue();
                    natural += glue.width;
                    stretch[static_cast<std::size_t>(glue.expand)] += glue.stretch;
                    shrink[static_cast<std::size_t>(glue.limit)] += glue.shrink;
                    break;
                }
                case Node::Type::Kern: {
                    natural += node->kern().width;
                    break;
                }
                case Node::Type::Rule: {
                    const auto& rule = node->rule();
                    natural += rule.width;
                    height = std::max(height, rule.height);
                    depth = std::max(depth, rule.depth);
                    break;
                }
                default:
                    break;
            }
        }

        float ratio = 0.0f;
        auto sign = Node::Sign::None;

        if (target > 0.0f) {
            if (natural < target) {
                const float gap = target - natural;
                std::size_t rank = 0;
                for (std::size_t order = 3; order > 0; --order) {
                    if (stretch[order] > 0.0f) {
                        rank = order;
                        break;
                    }
                }
                if (const float flex = stretch[rank]; flex > 0.0f) {
                    ratio = gap / flex;
                    sign = Node::Sign::Stretching;
                }
            } else if (natural > target) {
                const float gap = natural - target;
                std::size_t rank = 0;
                for (std::size_t order = 3; order > 0; --order) {
                    if (shrink[order] > 0.0f) {
                        rank = order;
                        break;
                    }
                }
                float flex = shrink[rank];
                if (flex > 0.0f) {
                    ratio = gap / flex;
                    sign = Node::Sign::Shrinking;
                }
            }
        }

        Node::Box data{
            .width = (target > 0.0f) ? target : natural,
            .height = height,
            .depth = depth,
            .shift = 0.0f,
            .ratio = ratio,
            .sign = sign,
            .alignment = Node::Alignment::Horizontal,
            .list = nodes
        };

        box->box(data);
        return box;
    }

    Node* Line::vertical(
        memory::Arena& arena,
        memory::Slice<Node*> nodes,
        const float skip
    ) noexcept {
        auto* box = arena.compose<Node>(Node::Type::Box);

        float height = 0.0f;
        float width = 0.0f;

        const std::size_t count = nodes.size();
        const std::size_t total = count + (count > 1 ? count - 1 : 0);

        auto slice = arena.allocate<Node*>(total);
        std::size_t mark = 0;

        for (std::size_t step = 0; step < count; ++step) {
            Node* child = nodes[step];
            if (!child) continue;

            if (child->type() == Node::Type::Box) {
                width = std::max(width, child->box().width);
                height += child->box().height + child->box().depth;
            }

            slice[mark++] = child;

            if (skip > 0.0f && step + 1 < count) {
                auto* glue = arena.compose<Node>(Node::Type::Glue);
                glue->glue({ .width = skip, .stretch = 0.0f, .shrink = 0.0f });
                slice[mark++] = glue;
                height += skip;
            }
        }

        const Node::Box data{
            .width = width,
            .height = height,
            .depth = 0.0f,
            .shift = 0.0f,
            .ratio = 0.0f,
            .sign = Node::Sign::None,
            .alignment = Node::Alignment::Vertical,
            .list = slice
        };

        box->box(data);
        return box;
    }

}