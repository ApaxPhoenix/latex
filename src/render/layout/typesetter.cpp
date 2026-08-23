#include "layout/typesetter.hpp"

namespace render::layout {

    Typesetter::Typesetter(
        memory::Arena& arena,
        memory::Arena& scratch
    ) noexcept
        : arena(arena), scratch(scratch) {}

    Typesetter::Typesetter(
        memory::Arena& arena,
        memory::Arena& scratch,
        const Settings& settings
    ) noexcept
        : arena(arena), scratch(scratch), settings(settings) {}

    Node* Typesetter::stack(memory::Slice<Node*> input) const noexcept {
        const std::size_t count = input.size();
        if (count == 0) return nullptr;

        const std::size_t total = count * 2;
        auto slice = scratch.allocate<Node*>(total);
        std::size_t mark = 0;

        float depth = 0.0f;

        for (std::size_t step = 0; step < count; ++step) {
            Node* child = input[step];
            if (!child) continue;

            if (child->type() == Node::Type::Box) {
                const auto& box = child->box();
                if (step > 0) {
                    const float gap = settings.baseline - (depth + box.height);
                    auto* glue = arena.compose<Node>(Node::Type::Glue);
                    if (gap >= settings.limit) {
                        glue->glue({ .width = gap, .stretch = 0.0f, .shrink = 0.0f });
                    } else {
                        glue->glue({ .width = settings.skip, .stretch = 0.0f, .shrink = 0.0f });
                    }
                    slice[mark++] = glue;
                }
                depth = box.depth;
            }

            slice[mark++] = child;
        }

        auto result = arena.allocate<Node*>(mark);
        for (std::size_t step = 0; step < mark; ++step) {
            result[step] = slice[step];
        }

        return Line::vertical(arena, result, 0.0f);
    }

    memory::Slice<Pager::Page> Typesetter::compose(Document& document) const {
        document.layout();

        const memory::Slice<Paragraph*> paragraphs = document.paragraphs();
        const std::size_t count = paragraphs.size();
        if (count == 0) return {};

        auto slice = scratch.allocate<Node*>(count);
        std::size_t mark = 0;

        for (std::size_t step = 0; step < count; ++step) {
            const Paragraph* paragraph = paragraphs[step];
            if (!paragraph) continue;
            if (Node* box = paragraph->node()) {
                slice[mark++] = box;
            }
        }

        auto input = arena.allocate<Node*>(mark);
        for (std::size_t step = 0; step < mark; ++step) {
            input[step] = slice[step];
        }

        const Node* root = stack(input);
        if (!root) return {};

        const Pager pager(arena);
        const Pager::Context context{ .height = document.configuration().height };
        return pager.paginate(root, context);
    }

}