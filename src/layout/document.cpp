#include "layout/document.hpp"
#include "layout/breaker.hpp"
#include <algorithm>

namespace layout {

    Document::Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper) noexcept
        : arena(arena), pager(pager), shaper(shaper) {}

    Document::Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper, const Configuration& configuration) noexcept
        : arena(arena), pager(pager), shaper(shaper), configuration_(configuration) {}

    void Document::margin(float left, float right, float top, float bottom) noexcept {
        configuration_.left = left;
        configuration_.right = right;
        configuration_.top = top;
        configuration_.bottom = bottom;
    }

    void Document::paper(float width, float height) noexcept {
        configuration_.width = width;
        configuration_.height = height;
    }

    void Document::grid(std::int32_t columns, float gap) noexcept {
        configuration_.columns = columns;
        configuration_.gap = gap;
    }

    void Document::font(float size, float leading) noexcept {
        configuration_.size = size;
        configuration_.leading = leading;
    }

    void Document::indent(float value) noexcept {
        configuration_.indent = value;
    }

    void Document::spacing(float value) noexcept {
        configuration_.spacing = value;
    }

    void Document::gap(float value) noexcept {
        configuration_.gap = value;
    }

    void Document::align(std::int32_t value) noexcept {
        configuration_.align = value;
    }

    void Document::configure(const Configuration& configuration) noexcept {
        configuration_ = configuration;
    }

    const Document::Configuration& Document::configuration() const noexcept {
        return configuration_;
    }

    float Document::measure() const noexcept {
        float width = configuration_.width - (configuration_.left + configuration_.right);
        if (configuration_.columns > 1) {
            const float gap = configuration_.gap * static_cast<float>(configuration_.columns - 1);
            width = (width - gap) / static_cast<float>(configuration_.columns);
        }
        return std::max(0.0f, width);
    }

    const std::vector<Node*>& Document::content() const noexcept {
        return lines;
    }

    std::vector<Node*> Document::tokenize(const typography::Font& font, std::string_view text) const {
        std::vector<Node*> nodes;

        for (const char letter : text) {
            if (letter == ' ') {
                Node* const node = arena.compose<Node>();
                const Node::Glue glue{
                    .width = configuration_.size * 0.25f,
                    .stretch = configuration_.size * 0.125f,
                    .shrink = configuration_.size * 0.083f,
                    .stretchorder = Node::Order::Normal,
                    .shrinkorder = Node::Order::Normal
                };
                node->glue(glue);
                nodes.push_back(node);
            } else if (letter == '\n') {
                Node* const node = arena.compose<Node>();
                constexpr Node::Break penalty{
                    .penalty = Node::Penalty{-10000}
                };
                node->breaks(penalty);
                nodes.push_back(node);
            } else {
                Node* const node = arena.compose<Node>();
                const Node::Glyph glyph{
                    .width = configuration_.size * 0.6f,
                    .height = configuration_.size * 0.8f,
                    .depth = configuration_.size * 0.2f,
                    .code = static_cast<std::uint32_t>(letter)
                };
                node->glyph(glyph);
                nodes.push_back(node);
            }
        }

        return nodes;
    }

    void Document::append(const typography::Font& font, std::string_view text) {
        const auto tokens = tokenize(font, text);
        lines.insert(lines.end(), tokens.begin(), tokens.end());
    }

    void Document::append(Node* node) {
        if (node) {
            lines.push_back(node);
        }
    }

    void Document::append(memory::Slice<Node*> slice) {
        for (std::size_t index = 0; index < slice.size(); ++index) {
            if (slice[index]) {
                lines.push_back(slice[index]);
            }
        }
    }

    void Document::newline() {
        Node* const node = arena.compose<Node>();
        constexpr Node::Break penalty{
            .penalty = Node::Penalty{-10000}
        };
        node->breaks(penalty);
        lines.push_back(node);
    }

    memory::Slice<Node*> Document::compose() const {
        if (lines.empty()) return memory::Slice<Node*>{};

        const Breaker::Configuration config{
            .target = measure(),
            .leading = configuration_.leading,
            .tolerance = 2000.0,
            .penalty = 0.0,
            .skip = configuration_.spacing,
            .limit = 0.0
        };

        Breaker breaker(arena, config);
        const memory::Slice<Node*> slice(const_cast<Node**>(lines.data()), lines.size());
        return breaker.compose(slice);
    }

    memory::Slice<Document::Page> Document::split() const {
        const memory::Slice<Node*> items = compose();
        if (items.empty()) return memory::Slice<Document::Page>{};

        for (std::size_t index = 0; index + 1 < items.size(); ++index) {
            items[index]->next(items[index + 1]);
        }
        items[items.size() - 1]->next(nullptr);

        const Pager::Context context{
            .height = configuration_.height - (configuration_.top + configuration_.bottom),
            .width = measure(),
            .skip = configuration_.spacing
        };

        return pager.paginate(items[0], context);
    }

    void Document::reset() noexcept {
        lines.clear();
        pages.clear();
    }

}