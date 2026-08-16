#pragma once

#include "layout/node.hpp"

#include <include/core/SkBitmap.h>
#include <cstdint>

namespace render {

    class Wasm {
    public:
        Wasm() noexcept = default;

        void compose(const layout::Node* page, int width, int height);
        void snippet(const layout::Node* node);

        [[nodiscard]] const std::uint8_t* pixels() const noexcept;
        [[nodiscard]] int width() const noexcept;
        [[nodiscard]] int height() const noexcept;

    private:
        SkBitmap bitmap{};
    };

}