#include "layout/document.hpp"
#include "logger.hpp"

namespace layout {

    Document::Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper) noexcept
        : arena(arena), pager(pager), shaper(shaper) {}

    Document::Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper, const Configuration& configuration) noexcept
        : arena(arena), pager(pager), shaper(shaper), configuration_(configuration) {}

    void Document::margin(const float left, const float right, const float top, const float bottom) noexcept {
        configuration_.left = left;
        configuration_.right = right;
        configuration_.top = top;
        configuration_.bottom = bottom;
    }

    void Document::paper(const float width, const float height) noexcept {
        configuration_.width = width;
        configuration_.height = height;
    }

    void Document::grid(const std::int32_t columns, const float gap) noexcept {
        configuration_.columns = columns;
        configuration_.gap = gap;
    }

    void Document::font(const float size, const float leading) noexcept {
        configuration_.size = size;
        configuration_.leading = leading;
    }

    void Document::indent(const float value) noexcept {
        configuration_.indent = value;
    }

    void Document::spacing(const float value) noexcept {
        configuration_.spacing = value;
    }

    void Document::gap(const float value) noexcept {
        configuration_.gap = value;
    }

    void Document::align(const std::int32_t value) noexcept {
        configuration_.align = value;
    }

    void Document::configure(const Configuration& configuration) noexcept {
        configuration_ = configuration;
    }

    const Document::Configuration& Document::configuration() const noexcept {
        return configuration_;
    }

    float Document::measure() const noexcept {
        float total{0.0f};
        for (auto* node : lines) {
            if (!node) continue;
            if (node->type() == Node::Type::Box) {
                total += node->box().height;
            } else if (node->type() == Node::Type::Glyph) {
                total += node->glyph().height;
            } else if (node->type() == Node::Type::Rule) {
                total += node->rule().height;
            } else if (node->type() == Node::Type::Glue) {
                total += node->glue().width;
            }
        }
        return total;
    }

    const std::vector<Node*>& Document::content() const noexcept {
        return lines;
    }

    std::vector<Node*> Document::tokenize(const typography::Font& font, const std::string_view text) const {
        const auto slice = shaper.shape(font, text);
        if (slice.empty() || !slice.data) return {};

        for (std::size_t index = 0; index + 1 < slice.size(); ++index) {
            if (slice[index]) {
                slice[index]->next(slice[index + 1]);
            }
        }
        return {slice.data, slice.data + slice.size()};
    }

    void Document::append(const typography::Font& font, const std::string_view text) {
        if (auto tokens = tokenize(font, text); !tokens.empty()) {
            if (!lines.empty() && lines.back()) {
                lines.back()->next(tokens.front());
            }
            lines.insert(lines.end(), tokens.begin(), tokens.end());
        }
    }

    void Document::newline() {
        Node* node = arena.compose<Node>(Node::Type::Glue);
        node->glue({
            .width = configuration_.leading
        });

        if (!lines.empty() && lines.back()) {
            lines.back()->next(node);
        }
        lines.push_back(node);
    }

    memory::Slice<Node*> Document::compose() const {
        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Composing document slice across {} line node(s)", lines.size());
        const memory::Slice<Node*> slice = arena.allocate<Node*>(lines.size());
        for (std::size_t index = 0; index < lines.size(); ++index) {
            slice.data[index] = lines[index];
        }
        return slice;
    }

    memory::Slice<Page> Document::split() const {
        if (lines.empty()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Warn, "Document split requested on empty content stream");
            return {};
        }
        Logger::fmt(Logger::Type::Layout, Logger::Level::Info,
                    "Splitting document stream starting from head node");
        return pager.paginate(lines.front(), {
            .height = configuration_.height - configuration_.top - configuration_.bottom,
            .width = configuration_.width - configuration_.left - configuration_.right,
            .skip = configuration_.spacing
        });
    }

    void Document::reset() noexcept {
        Logger::log(Logger::Type::Layout, Logger::Level::Debug, "Resetting document lines and pages");
        lines.clear();
        pages.clear();
    }

}