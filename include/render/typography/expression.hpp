#pragma once

#include "typography/font.hpp"

namespace typography {

    class Expression {
    public:
        struct Metrics {
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

        explicit Expression(const Font& font, float divisor = 64.0f) noexcept;

        [[nodiscard]] std::uint32_t glyph(std::uint32_t code) const noexcept;
        [[nodiscard]] Variant scale(std::uint32_t glyph, float height, float divisor = 64.0f) const noexcept;
        [[nodiscard]] float kern(std::uint32_t left, std::uint32_t right, float divisor = 64.0f) const noexcept;
        [[nodiscard]] constexpr const Metrics& metrics() const noexcept { return data; }

    private:
        const Font& parent;
        Metrics data{};
    };

}