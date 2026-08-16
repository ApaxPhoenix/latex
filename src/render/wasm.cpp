#include "render/wasm.hpp"
#include "render/painter.hpp"

#include <include/core/SkCanvas.h>
#include <algorithm>

namespace render {

    void Wasm::compose(const layout::Node* page, const int width, const int height) {
        if (!page || width <= 0 || height <= 0) return;

        bitmap.allocN32Pixels(width, height);
        SkCanvas canvas(bitmap);
        canvas.clear(SK_ColorWHITE);

        Painter painter(&canvas);
        painter.compose(page);
    }

    void Wasm::snippet(const layout::Node* node) {
        if (!node) return;

        struct Bound {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        Bound bound{};
        switch (node->type()) {
            case layout::Node::Type::Box:
                bound = {node->box().width, node->box().height, node->box().depth};
                break;
            case layout::Node::Type::Glyph:
                bound = {node->glyph().width, node->glyph().height, node->glyph().depth};
                break;
            case layout::Node::Type::Rule:
                bound = {node->rule().width, node->rule().height, node->rule().depth};
                break;
            default:
                break;
        }

        const int width = std::max(1, static_cast<int>(bound.width));
        const int height = std::max(1, static_cast<int>(bound.height + bound.depth));

        bitmap.allocN32Pixels(width, height);
        SkCanvas canvas(bitmap);
        canvas.clear(SK_ColorTRANSPARENT);

        Painter painter(&canvas);
        painter.compose(node, 0.0f, bound.height);
    }

    const std::uint8_t* Wasm::pixels() const noexcept {
        return static_cast<const std::uint8_t*>(bitmap.getPixels());
    }

    int Wasm::width() const noexcept {
        return bitmap.width();
    }

    int Wasm::height() const noexcept {
        return bitmap.height();
    }

}