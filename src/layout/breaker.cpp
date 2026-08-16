#include "layout/breaker.hpp"
#include "layout/line.hpp"

#include <algorithm>
#include <vector>

namespace layout {

    Breaker::Breaker(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena(arena), configuration(configuration) {}

    double Breaker::badness(const double delta, const double flex) noexcept {
        if (flex <= 0.0) return 10000.0;
        const double ratio = delta / flex;
        return 100.0 * ratio * ratio * ratio;
    }

    std::uint8_t Breaker::classify(const double ratio) noexcept {
        if (ratio < 0.5) return 0;
        if (ratio < 1.0) return 1;
        if (ratio < 2.0) return 2;
        return 3;
    }

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) const {
        if (input.empty()) return {};

        std::vector<Metrics> metrics(input.size() + 1uz);
        for (std::size_t index = 0uz; index < input.size(); ++index) {
            const auto* node = input[index];
            metrics[index + 1uz] = metrics[index];

            if (node->type() == Node::Type::Glyph) {
                metrics[index + 1uz].width += node->glyph().width;
            } else if (node->type() == Node::Type::Glue) {
                metrics[index + 1uz].width += node->glue().width;
                metrics[index + 1uz].stretch += node->glue().stretch;
                metrics[index + 1uz].shrink += node->glue().shrink;
            } else if (node->type() == Node::Type::Box) {
                metrics[index + 1uz].width += node->box().width;
            } else if (node->type() == Node::Type::Kern) {
                metrics[index + 1uz].width += node->kern().width;
            } else if (node->type() == Node::Type::Rule) {
                metrics[index + 1uz].width += node->rule().width;
            }
        }

        std::vector<Active> active;
        active.push_back(Active{.node = 0uz, .line = 0uz, .fitness = 0, .demerits = 0.0, .previous = 0uz});

        for (std::size_t index = 0uz; index < input.size(); ++index) {
            const auto* node = input[index];
            if (node->type() != Node::Type::Glue && node->type() != Node::Type::Penalty) continue;

            const std::size_t total = active.size();
            for (std::size_t point = 0uz; point < total; ++point) {
                const auto& item = active[point];
                const double delta = configuration.target - (metrics[index].width - metrics[item.node].width);
                const double stretch = metrics[index].stretch - metrics[item.node].stretch;
                const double shrink = metrics[index].shrink - metrics[item.node].shrink;

                const double flex = delta >= 0.0 ? stretch : shrink;
                const double ratio = flex > 0.0 ? delta / flex : 0.0;

                if (delta < 0.0 && shrink <= 0.0) continue;

                double penalty = 0.0;
                if (node->type() == Node::Type::Penalty) {
                    penalty = static_cast<double>(node->penalty().value);
                }

                const double score = badness(delta, flex) + penalty;
                const double demerits = item.demerits + score * score;

                active.push_back(Active{
                    .node = index,
                    .line = item.line + 1uz,
                    .fitness = classify(ratio),
                    .demerits = demerits,
                    .previous = point
                });
            }
        }

        if (active.empty()) return {};

        std::size_t best = 0uz;
        for (std::size_t index = 1uz; index < active.size(); ++index) {
            if (active[index].demerits < active[best].demerits) {
                best = index;
            }
        }

        std::vector<std::size_t> chain;
        for (std::size_t current = best; current != 0uz; current = active[current].previous) {
            chain.push_back(active[current].node);
        }
        std::ranges::reverse(chain);

        std::vector<Node*> result;
        std::size_t start = 0uz;

        for (const std::size_t finish : chain) {
            const std::size_t count = finish - start;
            if (count == 0uz) continue;

            memory::Slice<Node*> slice = arena.allocate<Node*>(count);
            for (std::size_t offset = 0uz; offset < count; ++offset) {
                slice[offset] = input[start + offset];
            }

            if (Node* box = Line::horizontal(arena, slice, static_cast<float>(configuration.target))) result.push_back(box);

            start = finish;
        }

        memory::Slice<Node*> lines = arena.allocate<Node*>(result.size());
        std::ranges::copy(result, lines.begin());
        return lines;
    }

}