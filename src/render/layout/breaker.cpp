#include "layout/breaker.hpp"
#include "layout/line.hpp"

#include <cmath>

namespace render::layout {

    Breaker::Breaker(memory::Arena& arena, memory::Arena& scratch, const Configuration& configuration) noexcept
        : arena(arena), scratch(scratch), configuration(configuration) {}

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) const {
        if (input.empty()) return {};

        struct Active {
            std::size_t index{0};
            std::size_t line{0};
            double demerits{0.0};
            const Active* link{nullptr};
        };

        const std::size_t count = input.size();
        const std::size_t capacity = count * 4 + 16;

        auto buffer = scratch.allocate<Active>(capacity);
        std::size_t active = 0;

        buffer[active++] = Active{
            .index = 0,
            .line = 0,
            .demerits = 0.0,
            .link = nullptr
        };

        const Active* tail = nullptr;

        for (std::size_t cursor = 0; cursor < count; ++cursor) {
            const Node* node = input[cursor];

            bool split = false;
            double cost = 0.0;

            if (node->type() == Node::Type::Penalty) {
                split = true;
                cost = static_cast<double>(node->penalty().value);
            } else if (node->type() == Node::Type::Glue && cursor > 0) {
                if (const Node* prior = input[cursor - 1]; prior->type() == Node::Type::Glyph ||
                    prior->type() == Node::Type::Box ||
                    prior->type() == Node::Type::Rule) {
                    split = true;
                }
            }

            if (cursor == count - 1) {
                split = true;
            }

            if (!split) continue;

            const Active* pick = nullptr;
            double lowest = 1e30;

            for (std::size_t slot = 0; slot < active; ++slot) {
                const auto& point = buffer[slot];

                double width = 0.0;
                double stretch = 0.0;
                double shrink = 0.0;

                for (std::size_t step = point.index; step <= cursor; ++step) {
                    if (const Node* item = input[step]; item->type() == Node::Type::Glyph) {
                        width += static_cast<double>(item->glyph().width);
                    } else if (item->type() == Node::Type::Box) {
                        width += static_cast<double>(item->box().width);
                    } else if (item->type() == Node::Type::Glue) {
                        width += static_cast<double>(item->glue().width);
                        stretch += static_cast<double>(item->glue().stretch);
                        shrink += static_cast<double>(item->glue().shrink);
                    } else if (item->type() == Node::Type::Kern) {
                        width += static_cast<double>(item->kern().width);
                    }
                }

                const double gap = configuration.target - width;
                double ratio = 0.0;
                double badness = 0.0;

                if (gap > 0.0) {
                    ratio = stretch > 0.0 ? gap / stretch : 100.0;
                    badness = 100.0 * std::pow(ratio, 3.0);
                } else if (gap < 0.0) {
                    ratio = shrink > 0.0 ? -gap / shrink : 100.0;
                    badness = ratio <= 1.0 ? 100.0 * std::pow(ratio, 3.0) : 10000.0;
                }

                if (badness > configuration.tolerance) continue;

                const double bias = configuration.penalty + cost;

                if (const double loss = std::pow(10.0 + badness, 2.0) + std::pow(bias, 2.0) + point.demerits; loss < lowest) {
                    lowest = loss;
                    pick = &point;
                }
            }

            if (pick && active < capacity) {
                buffer[active] = Active{
                    .index = cursor + 1,
                    .line = pick->line + 1,
                    .demerits = lowest,
                    .link = pick
                };
                tail = &buffer[active];
                active++;
            }
        }

        if (!tail) {
            std::size_t start = 0;
            float width = 0.0f;
            const std::size_t bound = count / 5 + 1;
            auto list = scratch.allocate<Node*>(bound);
            std::size_t rows = 0;

            for (std::size_t cursor = 0; cursor < count; ++cursor) {
                const Node* node = input[cursor];
                float span = 0.0f;

                if (node->type() == Node::Type::Glyph) span = node->glyph().width;
                else if (node->type() == Node::Type::Glue) span = node->glue().width;
                else if (node->type() == Node::Type::Kern) span = node->kern().width;
                else if (node->type() == Node::Type::Box) span = node->box().width;

                if (width + span > configuration.target && cursor > start) {
                    const std::size_t length = cursor - start;
                    auto slice = scratch.allocate<Node*>(length);
                    for (std::size_t step = 0; step < length; ++step) {
                        slice[step] = input[start + step];
                    }
                    list[rows++] = Line::horizontal(arena, slice, static_cast<float>(configuration.target));
                    start = cursor;
                    width = 0.0f;
                }
                width += span;
            }

            if (start < count) {
                const std::size_t length = count - start;
                auto slice = scratch.allocate<Node*>(length);
                for (std::size_t step = 0; step < length; ++step) {
                    slice[step] = input[start + step];
                }
                list[rows++] = Line::horizontal(arena, slice, static_cast<float>(configuration.target));
            }

            auto result = arena.allocate<Node*>(rows);
            for (std::size_t cursor = 0; cursor < rows; ++cursor) {
                result[cursor] = list[cursor];
            }
            return result;
        }

        const std::size_t lines = tail->line;
        auto result = arena.allocate<Node*>(lines);

        const Active* current = tail;
        while (current && current->link) {
            const std::size_t row = current->line - 1;
            const std::size_t start = current->link->index;
            const std::size_t end = current->index;

            if (const std::size_t length = end > start ? end - start : 0; length > 0) {
                auto slice = arena.allocate<Node*>(length);
                for (std::size_t step = 0; step < length; ++step) {
                    slice[step] = input[start + step];
                }
                result[row] = Line::horizontal(arena, slice, static_cast<float>(configuration.target));
            }
            current = current->link;
        }

        return result;
    }

}