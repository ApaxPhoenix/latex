#include "typography/expression.hpp"
#include <harfbuzz/hb-ot.h>

namespace typography {

    Expression::Expression(const Font& font, const float divisor) noexcept : parent(font) {
        hb_font_t* handle = font.hb();
        if (!handle || divisor == 0.0f) return;

        const float ratio = 1.0f / divisor;
        data.axis = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_AXIS_HEIGHT)) * ratio;
        data.fraction = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN)) * ratio;
        data.radical = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP)) * ratio;
        data.subscript = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN)) * ratio;
        data.superscript = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP)) * ratio;
        data.limit = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN)) * ratio;
    }

    std::uint32_t Expression::glyph(const std::uint32_t code) const noexcept {
        hb_font_t* handle = parent.hb();
        if (!handle) return 0;

        hb_codepoint_t result = 0;
        hb_font_get_nominal_glyph(handle, code, &result);
        return result;
    }

    Expression::Variant Expression::scale(const std::uint32_t glyph, const float height, const float divisor) const noexcept {
        hb_font_t* handle = parent.hb();
        if (!handle || divisor == 0.0f) return {};

        hb_ot_math_glyph_variant_t variants[8];
        unsigned int count = 8;

        hb_ot_math_get_glyph_variants(handle, glyph, HB_DIRECTION_TTB, 0, &count, variants);

        const float ratio = 1.0f / divisor;
        for (unsigned int index = 0; index < count; ++index) {
            if (static_cast<float>(variants[index].advance) * ratio >= height) {
                return Variant{.glyph = variants[index].glyph, .advance = static_cast<float>(variants[index].advance) * ratio};
            }
        }

        if (count > 0) {
            return Variant{.glyph = variants[count - 1].glyph, .advance = static_cast<float>(variants[count - 1].advance) * ratio};
        }

        return Variant{.glyph = glyph, .advance = 0.0f};
    }

    float Expression::kern(const std::uint32_t left, const std::uint32_t right, const float divisor) const noexcept {
        hb_font_t* handle = parent.hb();
        if (!handle || divisor == 0.0f) return 0.0f;

        const hb_position_t first = hb_ot_math_get_glyph_kerning(handle, left, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0);
        const hb_position_t second = hb_ot_math_get_glyph_kerning(handle, right, HB_OT_MATH_KERN_TOP_LEFT, 0);

        return static_cast<float>(first + second) * (1.0f / divisor);
    }

}