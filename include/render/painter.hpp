#pragma once

#include "layout/node.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>

namespace render {

    class Painter {
    public:
        explicit Painter(SkCanvas* canvas) noexcept;

        void draw(const layout::Node* root, float x = 0.0f, float y = 0.0f) const;

    private:
        void box(const layout::Node* node, float x, float y) const;
        void node(const layout::Node* node, float x, float y) const;
        void glyph(const layout::Node* node, float x, float y) const;
        void rule(const layout::Node* node, float x, float y) const;

        SkCanvas* canvas{nullptr};
        SkPaint paint{};
    };

}