#include "layout/pager.hpp"
#include <algorithm>
#include <cmath>

namespace layout {

    Pager::Pager(memory::Arena& arena) noexcept
        : arena(arena) {}

    Pager::Pager(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena(arena), configuration_(configuration) {}

    void Pager::configure(const Configuration& configuration) noexcept {
        configuration_ = configuration;
    }

    const Pager::Configuration& Pager::configuration() const noexcept {
        return configuration_;
    }

    std::int32_t Pager::badness(const float actual, const float target, const float flex) noexcept {
        const float delta = target - actual;
        if (delta == 0.0f) return 0;
        if (flex <= 0.0f) return 10000;
        const float ratio = delta / flex;
        if (ratio < -1.0f) return 10000;
        return static_cast<std::int32_t>(std::min(10000.0f, 100.0f * std::pow(std::abs(ratio), 3.0f)));
    }

    memory::Slice<Node*> Pager::split(Node* head, float target) const {
        if (!head) return memory::Slice<Node*>{};

        std::size_t count = 0;
        float total = 0.0f;
        Node* current = head;

        while (current) {
            float height = 0.0f;
            if (current->type() == Node::Type::Box) {
                height = current->box().height + current->box().depth;
            } else if (current->type() == Node::Type::Glue) {
                height = current->glue().width;
            } else if (current->type() == Node::Type::Rule) {
                height = current->rule().height + current->rule().depth;
            }

            if (total + height > target && count > 0) {
                break;
            }

            total += height;
            ++count;
            current = current->next();
        }

        if (count == 0) return memory::Slice<Node*>{};

        auto buffer = arena.allocate<Node*>(count);
        current = head;
        for (std::size_t index = 0; index < count; ++index) {
            buffer[index] = current;
            current = current->next();
        }

        return buffer;
    }

    memory::Slice<Pager::Page> Pager::paginate(Node* head, const Context& context) const {
        if (!head) return memory::Slice<Page>{};

        const float target = context.height > 0.0f ? context.height : configuration_.target;

        std::size_t pages = 0;
        Node* current = head;
        while (current) {
            std::size_t nodes = 0;
            float height = 0.0f;

            while (current) {
                float space = 0.0f;
                if (current->type() == Node::Type::Box) {
                    space = current->box().height + current->box().depth;
                } else if (current->type() == Node::Type::Glue) {
                    space = current->glue().width;
                } else if (current->type() == Node::Type::Rule) {
                    space = current->rule().height + current->rule().depth;
                }

                if (height + space > target && nodes > 0) {
                    if (current->type() == Node::Type::Penalty && current->penalty().value < 0) {
                        current = current->next();
                    }
                    break;
                }

                height += space;
                ++nodes;
                current = current->next();
            }

            if (nodes == 0 && current) {
                current = current->next();
            }

            ++pages;
        }

        if (pages == 0) return memory::Slice<Page>{};

        auto result = arena.allocate<Page>(pages);

        current = head;
        std::int32_t index = 0;

        for (std::size_t i = 0; i < pages && current; ++i) {
            Node* start = current;
            std::size_t nodes = 0;
            float height = 0.0f;

            while (current) {
                float space = 0.0f;
                if (current->type() == Node::Type::Box) {
                    space = current->box().height + current->box().depth;
                } else if (current->type() == Node::Type::Glue) {
                    space = current->glue().width;
                } else if (current->type() == Node::Type::Rule) {
                    space = current->rule().height + current->rule().depth;
                }

                if (height + space > target && nodes > 0) {
                    if (current->type() == Node::Type::Penalty && current->penalty().value < 0) {
                        height += space;
                        ++nodes;
                        current = current->next();
                    }
                    break;
                }

                height += space;
                ++nodes;
                current = current->next();
            }

            auto buffer = arena.allocate<Node*>(nodes);
            Node* cursor = start;
            for (std::size_t j = 0; j < nodes; ++j) {
                buffer[j] = cursor;
                cursor = cursor->next();
            }

            result[i] = Page{
                .nodes = buffer,
                .height = height,
                .index = index++,
                .badness = badness(height, target, 20.0f)
            };
        }

        return result;
    }

}