#pragma once

#include "typography/font.hpp"

#include <cstdint>

namespace render::typography {

    class Expression {
    public:
        struct Metric {
            float axis{0.0f};
            float fraction{0.0f};
            float radical{0.0f};
            float subscript{0.0f};
            float superscript{0.0f};
            float limit{0.0f};
        };

        struct Variant {
            std::uint32_t glyph{0};
            float advance{0.0f};
        };

        explicit Expression(const Font& font) noexcept;

        [[nodiscard]] Metric metrics() const noexcept;
        [[nodiscard]] std::uint32_t glyph(std::uint32_t code) const noexcept;
        [[nodiscard]] Variant scale(std::uint32_t glyph, float height) const noexcept;
        [[nodiscard]] float kern(std::uint32_t first, std::uint32_t second) const noexcept;

    private:
        const Font& parent;
    };

}