#include "layout/paragraph.hpp"
#include "layout/breaker.hpp"
#include "layout/line.hpp"

namespace render::layout {

    Paragraph::Paragraph(
        memory::Arena& arena,
        const std::string_view text,
        const typography::Font& font,
        const float size
    ) noexcept
        : arena(arena), content(text), face(&font), scale(size) {}

    void Paragraph::assign(const std::string_view text) noexcept {
        if (content != text) {
            content = text;
            stale = true;
        }
    }

    void Paragraph::touch() noexcept {
        stale = true;
    }

    bool Paragraph::dirty() const noexcept {
        return stale;
    }

    float Paragraph::height() const noexcept {
        return tall;
    }

    float Paragraph::offset() const noexcept {
        return shift;
    }

    void Paragraph::offset(const float value) noexcept {
        shift = value;
    }

    Node* Paragraph::node() const noexcept {
        return tree;
    }

    float Paragraph::layout(
        const typography::Shaper& shaper,
        Cache& cache,
        memory::Arena& scratch,
        const float width,
        const float leading
    ) noexcept {
        if (!stale) return 0.0f;

        const Cache::Key key{ .font = face, .text = content, .size = scale };
        memory::Slice<Node*> nodes = cache.find(key);

        if (nodes.empty()) {
            const typography::Font* fonts[] = { face };
            nodes = shaper.shape(
                memory::Slice{fonts, 1},
                content,
                {}
            );
            if (!nodes.empty()) {
                cache.insert(key, nodes);
            }
        }

        Breaker::Configuration configuration{
            .target = static_cast<double>(width),
            .leading = static_cast<double>(leading),
            .tolerance = 2000.0,
            .penalty = 0.0,
            .skip = static_cast<double>(leading),
            .limit = 0.0
        };

        const Breaker breaker(arena, scratch, configuration);
        const memory::Slice<Node*> lines = breaker.compose(nodes);

        tree = Line::vertical(arena, lines, leading);

        float updated = 0.0f;
        if (tree) {
            updated = tree->box().height;
        }

        const float delta = updated - tall;
        tall = updated;
        stale = false;

        return delta;
    }

}