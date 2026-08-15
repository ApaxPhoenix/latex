#include "layout/breaker.hpp"
#include "logger.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace layout {

    Breaker::Breaker(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena_(arena), configuration_(configuration) {
        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Breaker initialized with target={:.2f}pt, leading={:.2f}pt",
                    configuration_.target, configuration_.leading);
    }

    double Breaker::badness(const double delta, const double flex) noexcept {
        if (delta == 0.0) return 0.0;
        if (flex <= 0.0) return 10000.0;
        if ((delta / flex) < -1.0) return 10000.0;
        return std::min(100.0 * std::pow(std::abs(delta / flex), 3), 10000.0);
    }

    std::uint8_t Breaker::classify(const double ratio) noexcept {
        if (ratio < -0.5) return 0;
        if (ratio <= 0.5) return 1;
        if (ratio <= 1.0) return 2;
        return 3;
    }

    memory::Slice<Node*> Breaker::compose(memory::Slice<Node*> input) {
        if (input.empty()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Warn, "Breaker received empty paragraph stream");
            return {};
        }

        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Starting paragraph line-breaking pass on {} nodes...", input.size());

        std::vector<Node*> stream(input.begin(), input.end());

        Node* fill = arena_.compose<Node>(Node::Type::Glue);
        fill->glue({
            .width = 0.0f,
            .stretch = static_cast<float>(configuration_.target),
            .shrink = 0.0f,
            .stretchorder = Node::Order::Fil
        });
        stream.push_back(fill);

        Node* stop = arena_.compose<Node>(Node::Type::Penalty);
        stop->penalty({
            .value = -10000
        });
        stream.push_back(stop);

        const std::size_t count = stream.size();
        std::vector<Metrics> sums(count + 1);

        for (std::size_t index = 0; index < count; ++index) {
            sums[index + 1] = sums[index];
            if (const auto* node = stream[index]) {
                if (node->type() == Node::Type::Box) {
                    sums[index + 1].width += static_cast<double>(node->box().width);
                } else if (node->type() == Node::Type::Glyph) {
                    sums[index + 1].width += static_cast<double>(node->glyph().width);
                } else if (node->type() == Node::Type::Glue) {
                    sums[index + 1].width += static_cast<double>(node->glue().width);
                    sums[index + 1].stretch += static_cast<double>(node->glue().stretch);
                    sums[index + 1].shrink += static_cast<double>(node->glue().shrink);
                } else if (node->type() == Node::Type::Kern) {
                    sums[index + 1].width += static_cast<double>(node->kern().width);
                } else if (node->type() == Node::Type::Rule) {
                    sums[index + 1].width += static_cast<double>(node->rule().width);
                }
            }
        }

        std::vector<Active> active{
            Active{
                .node = 0,
                .line = 0,
                .fitness = 1,
                .demerits = 0.0,
                .previous = 0
            }
        };

        for (std::size_t current = 0; current < count; ++current) {
            const auto* node = stream[current];
            if (!((node->type() == Node::Type::Glue && current > 0 && stream[current - 1]->type() != Node::Type::Glue) ||
                  (node->type() == Node::Type::Penalty && node->penalty().value < 10000))) {
                continue;
            }

            for (std::size_t index = 0; index < active.size(); ++index) {
                const auto [start, line, prior, base, _] = active[index];
                const double delta = configuration_.target - (sums[current].width - sums[start].width);
                const double flex = (delta >= 0.0) ? (sums[current].stretch - sums[start].stretch)
                                                  : (sums[current].shrink - sums[start].shrink);

                const double score = badness(delta, flex);
                if (score > configuration_.tolerance && (node->type() != Node::Type::Penalty || node->penalty().value != -10000)) {
                    continue;
                }

                const int cost = (node->type() == Node::Type::Penalty) ? node->penalty().value : 0;
                double demerits = std::pow(configuration_.penalty + score, 2) +
                                  ((cost >= 0) ? std::pow(cost, 2) : (cost > -10000 ? -std::pow(cost, 2) : 0.0));

                const std::uint8_t fitness = classify(delta / (flex > 0.0 ? flex : 1.0));
                if (std::abs(static_cast<int>(fitness) - static_cast<int>(prior)) > 1) {
                    demerits += 10000.0;
                }

                active.push_back(Active{
                    .node = current,
                    .line = line + 1,
                    .fitness = fitness,
                    .demerits = base + demerits,
                    .previous = index
                });

                Logger::fmt(Logger::Type::Layout, Logger::Level::Trace,
                            "Evaluated break candidate [node {} -> {}] (badness={:.1f}, demerits={:.1f})",
                            start, current, score, base + demerits);
            }
        }

        std::size_t best = 0;
        double minimum = std::numeric_limits<double>::max();

        for (std::size_t index = 0; index < active.size(); ++index) {
            if (active[index].node == count - 1 && active[index].demerits < minimum) {
                minimum = active[index].demerits;
                best = index;
            }
        }

        std::vector<std::pair<std::size_t, std::size_t>> breaks;
        for (std::size_t trace = best; trace > 0; trace = active[trace].previous) {
            breaks.push_back({active[active[trace].previous].node, active[trace].node});
        }
        std::ranges::reverse(breaks);

        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Breaker constructed {} optimal line(s) (demerits={:.2f})",
                    breaks.size(), minimum);

        std::vector<Node*> lines;
        lines.reserve(breaks.size() * 2);
        double depth = 0.0;

        for (std::size_t index = 0; index < breaks.size(); ++index) {
            const auto [start, end] = breaks[index];
            const std::size_t length = end - start;
            memory::Slice<Node*> slice = arena_.allocate<Node*>(length);

            double height = 0.0;
            double bottom = 0.0;

            for (std::size_t offset = 0; offset < length; ++offset) {
                Node* child = stream[start + offset];
                slice[offset] = child;
                if (child) {
                    if (child->type() == Node::Type::Box) {
                        height = std::max(height, static_cast<double>(child->box().height));
                        bottom = std::max(bottom, static_cast<double>(child->box().depth));
                    } else if (child->type() == Node::Type::Rule) {
                        height = std::max(height, static_cast<double>(child->rule().height));
                        bottom = std::max(bottom, static_cast<double>(child->rule().depth));
                    }
                }
            }

            Node* box = arena_.compose<Node>(Node::Type::Box);
            box->box({
                .width = static_cast<float>(configuration_.target),
                .height = static_cast<float>(height),
                .depth = static_cast<float>(bottom),
                .list = slice
            });

            if (index > 0) {
                const double clearance = configuration_.leading - depth - height;
                Node* glue = arena_.compose<Node>(Node::Type::Glue);
                glue->glue({
                    .width = static_cast<float>(clearance >= configuration_.limit ? clearance : configuration_.skip)
                });

                if (clearance >= configuration_.limit) {
                    Logger::fmt(Logger::Type::Layout, Logger::Level::Trace,
                                "Inserted interline glue (height={:.2f}pt)", clearance);
                } else {
                    Logger::fmt(Logger::Type::Layout, Logger::Level::Trace,
                                "Clearance below threshold; inserted fallback glue (height={:.2f}pt)", configuration_.skip);
                }
                lines.push_back(glue);
            }

            lines.push_back(box);
            depth = bottom;
        }

        memory::Slice<Node*> result = arena_.allocate<Node*>(lines.size());
        std::ranges::copy(lines, result.begin());
        return result;
    }

}