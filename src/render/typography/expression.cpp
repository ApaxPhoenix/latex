#include "typography/expression.hpp"
#include <harfbuzz/hb-ot.h>
#include <algorithm>
#include <limits>

namespace render::typography {

    Expression::Expression(const Font& font) noexcept : parent(font) {}

    Expression::Metric Expression::metrics() const noexcept {
        if (!parent.hb()) return {};

        hb_font_t* handle = parent.hb();
        return Metric{
            .axis = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_AXIS_HEIGHT)),
            .fraction = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP)),
            .radical = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP)),
            .subscript = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN)),
            .superscript = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP)),
            .limit = static_cast<float>(hb_ot_math_get_constant(handle, HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN))
        };
    }

    std::uint32_t Expression::glyph(const std::uint32_t code) const noexcept {
        if (!parent.hb()) return 0;
        std::uint32_t result = 0;
        return hb_font_get_nominal_glyph(parent.hb(), code, &result) ? result : 0;
    }

    Expression::Variant Expression::scale(const std::uint32_t glyph, const float height) const noexcept {
        if (!parent.hb()) return {};

        hb_font_t* handle = parent.hb();
        hb_ot_math_glyph_variant_t variants[8];
        unsigned int count = 8;

        hb_ot_math_get_glyph_variants(
            handle,
            glyph,
            HB_DIRECTION_TTB,
            0,
            &count,
            variants
        );

        for (unsigned int index = 0; index < count; ++index) {
            if (static_cast<float>(variants[index].advance) >= height) {
                return Variant{
                    .glyph = variants[index].glyph,
                    .advance = static_cast<float>(variants[index].advance)
                };
            }
        }

        return Variant{.glyph = glyph, .advance = height};
    }

    float Expression::kern(const std::uint32_t first, const std::uint32_t second) const noexcept {
        if (!parent.hb()) return 0.0f;

        const auto safe_height = std::min<std::uint32_t>(
            second,
            std::numeric_limits<hb_position_t>::max()
        );

        return static_cast<float>(hb_ot_math_get_glyph_kerning(
            parent.hb(),
            first,
            HB_OT_MATH_KERN_BOTTOM_RIGHT,
            static_cast<hb_position_t>(safe_height)
        ));
    }

}