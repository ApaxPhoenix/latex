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

namespace render {

    class Composer {
    public:
        Composer(
            memory::Arena& arena,
            memory::Arena& scratch,
            typography::Shaper& shaper,
            layout::Typesetter& typesetter,
            SkCanvas* canvas
        ) noexcept;

        void feed(memory::Slice<syntax::Node*> ast, const typography::Font& font, float size);
        void feed(const syntax::expression::Node* root, const typography::Font& font);

        void paint(float x = 0.0f, float y = 0.0f);

        void draw(const layout::Node* root, float x = 0.0f, float y = 0.0f) const;
        void draw(memory::Slice<layout::Node*> nodes, float x = 0.0f, float y = 0.0f) const;

        [[nodiscard]] layout::Document& document() noexcept { return doc; }

    private:
        void stack(memory::Slice<layout::Node*> nodes, float x, float y) const;
        void box(const layout::Node* item, float x, float y) const;
        void node(const layout::Node* item, float x, float y) const;
        void glyph(const layout::Node* item, float x, float y) const;
        void rule(const layout::Node* item, float x, float y) const;

        memory::Arena& arena;
        memory::Arena& scratch;
        typography::Shaper& shaper;
        layout::Typesetter& typesetter;
        layout::Document doc;

        SkCanvas* canvas{nullptr};
        SkPaint ink{};
        mutable std::unordered_map<const typography::Font*, SkFont> fonts{};
    };

}