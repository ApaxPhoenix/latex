#include "layout/pager.hpp"
#include "logger.hpp"

namespace layout {

    Pager::Pager(memory::Arena& arena) noexcept
        : arena_(arena) {}

    Pager::Pager(memory::Arena& arena, const Configuration& configuration) noexcept
        : arena_(arena), configuration_(configuration) {}

    void Pager::configure(const Configuration& configuration) noexcept {
        configuration_ = configuration;
    }

    const Pager::Configuration& Pager::configuration() const noexcept {
        return configuration_;
    }

    memory::Slice<Node*> Pager::split(Node* head, const float target) const {
        if (!head) {
            return {};
        }

        float sum{0.0f};
        std::size_t count{0};
        Node* current = head;

        while (current) {
            float height{0.0f};

            if (current->type() == Node::Type::Box) {
                height = current->box().height;
            } else if (current->type() == Node::Type::Rule) {
                height = current->rule().height;
            } else if (current->type() == Node::Type::Glyph) {
                height = current->glyph().height;
            } else if (current->type() == Node::Type::Glue) {
                height = current->glue().width;
            } else if (current->type() == Node::Type::Insertion) {
                height = current->insertion().height;
            }

            if (sum + height > target && count > 0) {
                break;
            }

            sum += height;
            ++count;
            current = current->next();
        }

        if (count == 0) {
            return {};
        }

        memory::Slice<Node*> slice = arena_.allocate<Node*>(count);
        current = head;
        for (std::size_t index = 0; index < count; ++index) {
            slice[index] = current;
            current = current->next();
        }

        return slice;
    }

    memory::Slice<Page> Pager::paginate(Node* head, const Context& context) const {
        if (!head) {
            Logger::log(Logger::Type::Layout, Logger::Level::Warn, "Pager received null head node");
            return {};
        }

        const float limit = context.height > 0.0f ? context.height : configuration_.target;

        std::size_t count{0};
        Node* current = head;

        while (current) {
            auto chunk = split(current, limit);
            if (chunk.empty()) {
                break;
            }
            ++count;
            current = chunk[chunk.size() - 1]->next();
        }

        if (count == 0) {
            Logger::log(Logger::Type::Layout, Logger::Level::Warn, "Pagination generated zero pages");
            return {};
        }

        memory::Slice<Page> pages = arena_.allocate<Page>(count);
        current = head;
        std::int32_t number{0};

        for (std::size_t index = 0; index < count; ++index) {
            auto chunk = split(current, limit);
            if (chunk.empty()) {
                break;
            }

            float height{0.0f};
            float flex{0.0f};

            for (auto* node : chunk) {
                if (!node) continue;
                if (node->type() == Node::Type::Box) {
                    height += node->box().height;
                } else if (node->type() == Node::Type::Rule) {
                    height += node->rule().height;
                } else if (node->type() == Node::Type::Glyph) {
                    height += node->glyph().height;
                } else if (node->type() == Node::Type::Glue) {
                    height += node->glue().width;
                    flex += node->glue().stretch;
                } else if (node->type() == Node::Type::Insertion) {
                    height += node->insertion().height;
                }
            }

            Page page{};
            page.nodes = chunk;
            page.height = height;
            page.index = number++;
            page.badness = badness(height, limit, flex);

            pages[index] = page;
            current = chunk[chunk.size() - 1]->next();
        }

        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Pager generated {} page(s) (target height={:.2f}pt)",
                    count, limit);

        return pages;
    }

    std::int32_t Pager::badness(const float actual, const float target, const float flex) noexcept {
        if (target <= 0.0f) return 0;
        const float delta = actual - target;
        if (delta == 0.0f) return 0;

        const float difference = delta < 0.0f ? -delta : delta;
        if (flex <= 0.0f) {
            return 10000;
        }

        const float ratio = difference / flex;
        const float cubed = 100.0f * ratio * ratio * ratio;

        return static_cast<std::int32_t>(cubed > 10000.0f ? 10000.0f : cubed);
    }

}