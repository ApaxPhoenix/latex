#include "layout/pager.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

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

        std::vector<Node*> collected;
        float accumulated = 0.0f;
        Node* current = head;

        while (current != nullptr) {
            float height = 0.0f;

            if (current->type() == Node::Type::Box) {
                height = current->box().height + current->box().depth;
            } else if (current->type() == Node::Type::Glue) {
                height = current->glue().width;
            } else if (current->type() == Node::Type::Rule) {
                height = current->rule().height + current->rule().depth;
            }

            if (accumulated + height > target && !collected.empty()) {
                break;
            }

            accumulated += height;
            collected.push_back(current);
            current = current->next();
        }

        auto buffer = arena.allocate<Node*>(collected.size());
        for (std::size_t index = 0; index < collected.size(); ++index) {
            buffer[index] = collected[index];
        }
        return buffer;
    }

    memory::Slice<Pager::Page> Pager::paginate(Node* head, const Context& context) const {
        if (!head) return memory::Slice<Page>{};

        const float target = context.height > 0.0f ? context.height : configuration_.target;
        std::vector<Page> pages;

        Node* current = head;
        std::int32_t index = 0;

        while (current != nullptr) {
            std::vector<Node*> nodes;
            float height = 0.0f;

            while (current != nullptr) {
                float space = 0.0f;
                if (current->type() == Node::Type::Box) {
                    space = current->box().height + current->box().depth;
                } else if (current->type() == Node::Type::Glue) {
                    space = current->glue().width;
                } else if (current->type() == Node::Type::Rule) {
                    space = current->rule().height + current->rule().depth;
                }

                if (height + space > target && !nodes.empty()) {
                    if (current->type() == Node::Type::Penalty && current->penalty().value < 0) {
                        nodes.push_back(current);
                        height += space;
                        current = current->next();
                    }
                    break;
                }

                height += space;
                nodes.push_back(current);
                current = current->next();
            }

            auto buffer = arena.allocate<Node*>(nodes.size());
            for (std::size_t offset = 0; offset < nodes.size(); ++offset) {
                buffer[offset] = nodes[offset];
            }

            pages.push_back(Page{
                .nodes = buffer,
                .height = height,
                .index = index++,
                .badness = badness(height, target, 20.0f)
            });
        }

        auto buffer = arena.allocate<Page>(pages.size());
        for (std::size_t offset = 0; offset < pages.size(); ++offset) {
            buffer[offset] = pages[offset];
        }
        return buffer;
    }

}