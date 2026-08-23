#include "render/wasm.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

namespace render {

    void Wasm::compose(const layout::Node* node, int width, int height) {
        (void)node;

        width_ = width;
        height_ = height;

        #if defined(__EMSCRIPTEN__)
                if (node == nullptr) {
                    return;
                }

                EM_ASM({
                    if (typeof window !== 'undefined' && window.onCanvasRender) {
                        window.onCanvasRender($0, $1);
                    }
                }, width_, height_);
        #endif
    }

    int Wasm::width() const {
        return width_;
    }

    int Wasm::height() const {
        return height_;
    }

}