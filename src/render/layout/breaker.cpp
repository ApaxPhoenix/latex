#include "layout/breaker.hpp"
#include "layout/line.hpp"

#include <cmath>

namespace render::layout {

    Breaker::Breaker(memory::Arena& arena, memory::Arena& scratch, const Configuration& configuration) noexcept
        : arena(arena), scratch(scratch), configuration(configuration) {}

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) const {
        const std::size_t count = input.size();
        if (count == 0) return {};

        auto pref_w = scratch.allocate<double>(count + 1);
        auto pref_str = scratch.allocate<double>(count + 1);
        auto pref_shr = scratch.allocate<double>(count + 1);

        pref_w[0] = 0.0;
        pref_str[0] = 0.0;
        pref_shr[0] = 0.0;

        for (std::size_t step = 0; step < count; ++step) {
            const Node* item = input[step];
            double w = 0.0, str = 0.0, shr = 0.0;

            if (item->type() == Node::Type::Glyph) {
                w = static_cast<double>(item->glyph().width);
            } else if (item->type() == Node::Type::Box) {
                w = static_cast<double>(item->box().width);
            } else if (item->type() == Node::Type::Glue) {
                w = static_cast<double>(item->glue().width);
                str = static_cast<double>(item->glue().stretch);
                shr = static_cast<double>(item->glue().shrink);
            } else if (item->type() == Node::Type::Kern) {
                w = static_cast<double>(item->kern().width);
            }

            pref_w[step + 1] = pref_w[step] + w;
            pref_str[step + 1] = pref_str[step] + str;
            pref_shr[step + 1] = pref_shr[step] + shr;
        }

        struct Active {
            std::size_t index{0};
            std::size_t line{0};
            double demerits{0.0};
            const Active* link{nullptr};
        };

        const std::size_t capacity = count * 4 + 16;
        auto buffer = scratch.allocate<Active>(capacity);
        std::size_t active = 0;

        buffer[active++] = Active{ .index = 0, .line = 0, .demerits = 0.0, .link = nullptr };
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

                const double width = pref_w[cursor + 1] - pref_w[point.index];
                const double stretch = pref_str[cursor + 1] - pref_str[point.index];
                const double shrink = pref_shr[cursor + 1] - pref_shr[point.index];

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

        if (!tail) return {};

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