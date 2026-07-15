#include "pipeline/font.hpp"

#include <fontconfig/fontconfig.h>
#include <cairo-ft.h>
#include <ranges>

namespace pipeline::font {

    Font::Font() {
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        context = cairo_create(surface);
    }

    Font::~Font() {
        for (const auto &face: faces | std::views::values) {
            if (face) {
                cairo_font_face_destroy(face);
            }
        }
        if (context) cairo_destroy(context);
        if (surface) cairo_surface_destroy(surface);
    }

    void Font::load(const std::string& family, const std::string& variant, const std::string& path) {
        FcConfigAppFontAddFile(FcConfigGetCurrent(), reinterpret_cast<const FcChar8*>(path.c_str()));

        FcPattern* pattern = FcPatternCreate();
        FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(family.c_str()));
        FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN);
        FcPatternAddInteger(pattern, FC_WEIGHT, variant == "bold" ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM);

        FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcResult result;
        FcPattern* match = FcFontMatch(nullptr, pattern, &result);
        FcPatternDestroy(pattern);

        if (match) {
            cairo_font_face_t* face = cairo_ft_font_face_create_for_pattern(match);
            FcPatternDestroy(match);

            if (face) {
                faces[{hash(family, variant)}] = face;
            }
        }
    }

    float Font::width(const std::string& family, const std::string& variant, const std::string& text, const float size) const {
        if (text.empty()) return 0.0f;

        if (const auto lookup = faces.find({hash(family, variant)}); lookup != faces.end()) {
            cairo_set_font_face(context, lookup->second);
        } else {
            cairo_select_font_face(context, family.c_str(), CAIRO_FONT_SLANT_NORMAL,
                                   variant == "bold" ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        }

        cairo_set_font_size(context, size);

        cairo_text_extents_t metrics;
        cairo_text_extents(context, text.c_str(), &metrics);

        return static_cast<float>(metrics.x_advance);
    }

    float Font::height(const std::string& family, const std::string& variant, const float size) const {
        if (const auto lookup = faces.find({hash(family, variant)}); lookup != faces.end()) {
            cairo_set_font_face(context, lookup->second);
        } else {
            cairo_select_font_face(context, family.c_str(), CAIRO_FONT_SLANT_NORMAL,
                                   variant == "bold" ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        }

        cairo_set_font_size(context, size);

        cairo_font_extents_t metrics;
        cairo_font_extents(context, &metrics);

        return static_cast<float>(metrics.height);
    }

}
