#include "layout/breaker.hpp"
#include "layout/line.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace render::layout {

    namespace {
        constexpr std::int32_t kEjectPenalty = -10000;
        constexpr double kMaxBadness = 10000.0;
    }

    Breaker::Breaker(memory::Arena& arena, memory::Arena& scratch, const Configuration& configuration) noexcept
        : arena(arena), scratch(scratch), configuration(configuration) {}

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) const {
        const std::size_t count = input.size();
        if (count == 0) return {};

        auto width = scratch.allocate<double>(count + 1);
        auto stretch = scratch.allocate<double>(count + 1);
        auto shrink = scratch.allocate<double>(count + 1);

        width[0] = 0.0;
        stretch[0] = 0.0;
        shrink[0] = 0.0;

        for (std::size_t step = 0; step < count; ++step) {
            const Node* item = input[step];
            double w = 0.0, str = 0.0, shr = 0.0;

            switch (item->type()) {
                case Node::Type::Glyph: w = static_cast<double>(item->glyph().width); break;
                case Node::Type::Box:   w = static_cast<double>(item->box().width); break;
                case Node::Type::Kern:  w = static_cast<double>(item->kern().width); break;
                case Node::Type::Glue:
                    w = static_cast<double>(item->glue().width);
                    str = static_cast<double>(item->glue().stretch);
                    shr = static_cast<double>(item->glue().shrink);
                    break;
                default: break;
            }

            width[step + 1] = width[step] + w;
            stretch[step + 1] = stretch[step] + str;
            shrink[step + 1] = shrink[step] + shr;
        }

        struct Active {
            std::size_t index{0};
            std::size_t line{0};
            double demerits{0.0};
            const Active* link{nullptr};
        };

        const std::size_t capacity = count * 4 + 16;
        auto pool = scratch.allocate<Active>(capacity);
        std::size_t active = 0;

        pool[active++] = Active{.index = 0, .line = 0, .demerits = 0.0, .link = nullptr};
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

            // A forced break bypasses the tolerance gate below: the paragraph's
            // last position, or an explicit eject penalty, must always break.
            const bool last = cursor == count - 1;
            const bool eject = node->type() == Node::Type::Penalty && node->penalty().value <= kEjectPenalty;
            const bool forced = last || eject;
            if (forced) split = true;

            if (!split) continue;

            const Active* pick = nullptr;
            double lowest = std::numeric_limits<double>::max();

            for (std::size_t slot = 0; slot < active; ++slot) {
                const auto& point = pool[slot];

                const double span = width[cursor + 1] - width[point.index];
                const double give = stretch[cursor + 1] - stretch[point.index];
                const double take = shrink[cursor + 1] - shrink[point.index];
                const double gap = configuration.target - span;

                double ratio = 0.0;
                double badness = 0.0;

                if (gap > 0.0) {
                    ratio = give > 0.0 ? gap / give : 10.0;
                    badness = std::min(kMaxBadness, 100.0 * std::pow(ratio, 3.0));
                } else if (gap < 0.0) {
                    ratio = take > 0.0 ? -gap / take : 10.0;
                    badness = ratio <= 1.0 ? 100.0 * std::pow(ratio, 3.0) : kMaxBadness;
                }

                if (!forced && badness > configuration.tolerance) continue;

                const double bias = configuration.penalty + cost;
                if (const double loss = std::pow(10.0 + badness, 2.0) + std::pow(bias, 2.0) + point.demerits; loss < lowest) {
                    lowest = loss;
                    pick = &point;
                }
            }

            if (!pick || active >= capacity) continue;

            pool[active] = Active{.index = cursor + 1, .line = pick->line + 1, .demerits = lowest, .link = pick};
            tail = &pool[active];
            ++active;
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