#pragma once

#include "layout/node.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>

namespace render {

    class Painter {
    public:
        explicit Painter(SkCanvas* canvas) noexcept;

        void compose(const layout::Node* root, float x = 0.0f, float y = 0.0f);

    private:
        void character(const layout::Node* node, float x, float y) const;
        void rule(const layout::Node* node, float x, float y) const;
        void box(const layout::Node* node, float x, float y);

        SkCanvas* canvas{nullptr};
        SkPaint paint{};
        SkFont font{};
    };

}