#include "layout/breaker.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace layout {

    Breaker::Breaker(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena(arena), configuration(configuration) {}

    double Breaker::badness(const double delta, const double flex) noexcept {
        if (delta == 0.0) {
            return 0.0;
        }
        if (flex <= 0.0) {
            return 10000.0;
        }
        const double ratio = delta / flex;
        if (ratio < -1.0) {
            return 10000.0;
        }
        return std::min(10000.0, 100.0 * std::pow(std::abs(ratio), 3.0));
    }

    std::uint8_t Breaker::classify(const double ratio) noexcept {
        if (ratio < -0.5) return 0;
        if (ratio < 0.0)  return 1;
        if (ratio <= 0.5) return 2;
        return 3;
    }

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) const {
        if (input.empty()) {
            return memory::Slice<Node*>{};
        }

        std::vector<Active> active;
        active.push_back(Active{
            .node = 0,
            .line = 0,
            .fitness = 1,
            .demerits = 0.0,
            .previous = 0
        });

        std::vector<Node*> lines;
        Metrics metrics{};

        for (std::size_t index = 0; index < input.size(); ++index) {
            Node* current = input[index];
            if (!current) continue;

            switch (current->type()) {
                case Node::Type::Glyph: {
                    metrics.width += current->glyph().width;
                    break;
                }
                case Node::Type::Box: {
                    metrics.width += current->box().width;
                    break;
                }
                case Node::Type::Rule: {
                    metrics.width += current->rule().width;
                    break;
                }
                case Node::Type::Kern: {
                    metrics.width += current->kern().width;
                    break;
                }
                case Node::Type::Glue: {
                    const auto& item = current->glue();
                    metrics.width += item.width;
                    metrics.stretch += item.stretch;
                    metrics.shrink += item.shrink;
                    break;
                }
                case Node::Type::Penalty:
                case Node::Type::Break: {
                    if (metrics.width > 0.0) {
                        const double delta = configuration.target - metrics.width;
                        const double flex = delta >= 0.0 ? metrics.stretch : metrics.shrink;

                        if (const double value = badness(delta, flex); value < configuration.tolerance) {
                            Node* line = arena.compose<Node>();
                            const Node::Box box{
                                .width = static_cast<float>(metrics.width),
                                .height = static_cast<float>(configuration.leading),
                                .depth = 0.0f,
                                .list = memory::Slice<Node*>{input.data, index + 1}
                            };
                            line->box(box);
                            lines.push_back(line);

                            metrics = Metrics{};
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (metrics.width > 0.0 || lines.empty()) {
            Node* line = arena.compose<Node>();
            const Node::Box box{
                .width = static_cast<float>(metrics.width),
                .height = static_cast<float>(configuration.leading),
                .depth = 0.0f,
                .list = input
            };
            line->box(box);
            lines.push_back(line);
        }

        auto buffer = arena.allocate<Node*>(lines.size());
        for (std::size_t offset = 0; offset < lines.size(); ++offset) {
            buffer[offset] = lines[offset];
        }
        return buffer;
    }

}