#include "layout/pager.hpp"

#include <cmath>

namespace render::layout {

    Pager::Pager(memory::Arena& arena) noexcept
        : arena(arena) {}

    Pager::Pager(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena(arena), configuration(configuration) {}

    memory::Slice<Node*> Pager::split(const Node* head, const float target) const {
        if (!head || head->type() != Node::Type::Box) return {};

        const auto& list = head->box().list;
        float height = 0.0f;
        std::size_t mark = 0;

        for (std::size_t cursor = 0; cursor < list.size(); ++cursor) {
            const Node* child = list[cursor];
            if (!child) continue;

            float span = 0.0f;
            if (child->type() == Node::Type::Box) {
                span = child->box().height + child->box().depth;
            } else if (child->type() == Node::Type::Glue) {
                span = child->glue().width;
            }

            if (height + span > target && mark > 0) {
                break;
            }

            height += span;
            mark = cursor + 1;
        }

        auto slice = arena.allocate<Node*>(mark);
        for (std::size_t cursor = 0; cursor < mark; ++cursor) {
            slice[cursor] = list[cursor];
        }

        return slice;
    }

    memory::Slice<Pager::Page> Pager::paginate(const Node* head, const Context& context) const {
        if (!head || head->type() != Node::Type::Box) return {};

        const auto& list = head->box().list;
        const std::size_t total = list.size();
        if (total == 0) return {};

        auto pages = arena.allocate<Page>(total);

        std::size_t count = 0;
        std::size_t start = 0;
        float height = 0.0f;

        for (std::size_t step = 0; step < total; ++step) {
            const Node* child = list[step];
            if (!child) continue;

            float span = 0.0f;
            if (child->type() == Node::Type::Box) {
                span = child->box().height + child->box().depth;
            } else if (child->type() == Node::Type::Glue) {
                span = child->glue().width;
            }

            if (height + span > context.height && step > start) {
                const std::size_t length = step - start;
                auto slice = arena.allocate<Node*>(length);
                for (std::size_t cursor = 0; cursor < length; ++cursor) {
                    slice[cursor] = list[start + cursor];
                }

                const auto gap = static_cast<double>(context.height - height);
                const std::int32_t badness = static_cast<std::int32_t>(std::min(10000.0, 100.0 * std::pow(std::abs(gap), 3.0)));

                pages[count] = Page{
                    .nodes = slice,
                    .height = height,
                    .index = static_cast<std::int32_t>(count),
                    .badness = badness
                };
                count++;

                start = step;
                height = 0.0f;
            }

            height += span;
        }

        if (start < total) {
            const std::size_t length = total - start;
            auto slice = arena.allocate<Node*>(length);
            for (std::size_t cursor = 0; cursor < length; ++cursor) {
                slice[cursor] = list[start + cursor];
            }

            pages[count] = Page{
                .nodes = slice,
                .height = height,
                .index = static_cast<std::int32_t>(count),
                .badness = 0
            };
            count++;
        }

        auto result = arena.allocate<Page>(count);
        for (std::size_t cursor = 0; cursor < count; ++cursor) {
            result[cursor] = pages[cursor];
        }

        return result;
    }

}