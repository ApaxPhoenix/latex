#pragma once

#include "layout/document.hpp"
#include "layout/node.hpp"
#include "layout/typesetter.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "syntax/expression/node.hpp"
#include "syntax/node.hpp"
#include "typography/font.hpp"
#include "typography/shaper.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>

#include <unordered_map>
#include <vector>

namespace render {

    class Composer {
    public:
        Composer(
            memory::Arena& arena,
            memory::Arena& scratch,
            typography::Shaper& shaper,
            layout::Typesetter& setter,
            SkCanvas* board = nullptr
        ) noexcept;

        void feed(memory::Slice<syntax::Node*> nodes, const typography::Font& font, float size);
        void feed(const syntax::expression::Node* root, const typography::Font& font);

        void paint(float across = 0.0f, float down = 0.0f);

        void draw(const layout::Node* root, float across = 0.0f, float down = 0.0f) const;
        void draw(memory::Slice<layout::Node*> nodes, float across = 0.0f, float down = 0.0f) const;

        void target(SkCanvas* board) noexcept { canvas = board; }

        [[nodiscard]] layout::Document& document() noexcept { return paper; }
        [[nodiscard]] const layout::Document& document() const noexcept { return paper; }
        [[nodiscard]] layout::Typesetter& engine() const noexcept { return setter; }

    private:
        void stack(memory::Slice<layout::Node*> nodes, float across, float down) const;
        void box(const layout::Node* item, float across, float down) const;
        void node(const layout::Node* item, float across, float down) const;
        void glyph(const layout::Node* item, float across, float down) const;
        void rule(const layout::Node* item, float across, float down) const;
        void flush() const;

        memory::Arena& arena;
        memory::Arena& scratch;
        typography::Shaper& shaper;
        layout::Typesetter& setter;
        layout::Document paper;

        SkCanvas* canvas{nullptr};
        SkPaint ink{};

        mutable std::vector<SkGlyphID> glyphs{};
        mutable std::vector<SkPoint> spots{};
        mutable const typography::Font* cache{nullptr};
        mutable SkFont style{};
        mutable std::unordered_map<const typography::Font*, SkFont> styles{};
    };

}