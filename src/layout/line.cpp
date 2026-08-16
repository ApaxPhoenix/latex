#include "layout/line.hpp"

namespace layout {

    Node* Line::horizontal(memory::Arena& arena, memory::Slice<Node*> nodes, float target) noexcept {
        if (nodes.empty()) return nullptr;

        float width = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;

        for (std::size_t index = 0uz; index < nodes.size(); ++index) {
            const auto* node = nodes[index];
            if (!node) continue;

            if (node->type() == Node::Type::Glyph) {
                width += node->glyph().width;
                if (node->glyph().height > height) height = node->glyph().height;
                if (node->glyph().depth > depth) depth = node->glyph().depth;
            } else if (node->type() == Node::Type::Glue) {
                width += node->glue().width;
            } else if (node->type() == Node::Type::Box) {
                width += node->box().width;
                if (node->box().height > height) height = node->box().height;
                if (node->box().depth > depth) depth = node->box().depth;
            } else if (node->type() == Node::Type::Kern) {
                width += node->kern().width;
            } else if (node->type() == Node::Type::Rule) {
                width += node->rule().width;
                if (node->rule().height > height) height = node->rule().height;
                if (node->rule().depth > depth) depth = node->rule().depth;
            }
        }

        auto* box = arena.compose<Node>();
        box->box(Node::Box{
            .width = target > 0.0f ? target : width,
            .height = height,
            .depth = depth,
            .list = nodes
        });
        return box;
    }

    Node* Line::vertical(memory::Arena& arena, memory::Slice<Node*> nodes, float skip) noexcept {
        if (nodes.empty()) return nullptr;

        std::size_t count = nodes.size() * 2uz - 1uz;
        memory::Slice<Node*> list = arena.allocate<Node*>(count);

        float height = 0.0f;
        float width = 0.0f;
        std::size_t offset = 0uz;

        for (std::size_t index = 0uz; index < nodes.size(); ++index) {
            auto* node = nodes[index];
            if (!node) continue;

            if (node->type() == Node::Type::Box) {
                if (node->box().width > width) width = node->box().width;
                height += node->box().height + node->box().depth;
            }

            list[offset] = node;
            ++offset;

            if (index + 1uz < nodes.size()) {
                auto* glue = arena.compose<Node>();
                glue->glue(Node::Glue{.width = skip});
                height += skip;
                list[offset] = glue;
                ++offset;
            }
        }

        auto* box = arena.compose<Node>();
        box->box(Node::Box{
            .width = width,
            .height = height,
            .depth = 0.0f,
            .list = list
        });
        return box;
    }

}