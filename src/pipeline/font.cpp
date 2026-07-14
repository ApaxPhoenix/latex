#include "pipeline/font.hpp"

#include <cairo.h>
#include <fontconfig/fontconfig.h>

namespace pipeline {

    void Font::load(const std::string& family, const std::string& variant, const std::string& path) {
        FcConfigAppFontAddFile(FcConfigGetCurrent(), reinterpret_cast<const FcChar8*>(path.c_str()));
        this->registry[{family, variant}] = family;
    }

    float Font::width(const std::string& family, const std::string& variant, const std::string& text, const float size) const {
        if (text.empty()) return 0.0f;

        auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        auto* context = cairo_create(surface);

        std::string target = family;
        if (const auto lookup = this->registry.find({family, variant}); lookup != this->registry.end()) {
            target = lookup->second;
        }

        cairo_select_font_face(context, target.c_str(), CAIRO_FONT_SLANT_NORMAL,
                               variant == "bold" ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(context, size);

        cairo_text_extents_t metrics;
        cairo_text_extents(context, text.c_str(), &metrics);

        auto value = static_cast<float>(metrics.x_advance);

        cairo_destroy(context);
        cairo_surface_destroy(surface);

        return value;
    }

    float Font::height(const std::string& family, const std::string& variant, const float size) const {
        auto* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        auto* context = cairo_create(surface);

        std::string target = family;
        if (const auto lookup = this->registry.find({family, variant}); lookup != this->registry.end()) {
            target = lookup->second;
        }

        cairo_select_font_face(context, target.c_str(), CAIRO_FONT_SLANT_NORMAL,
                               variant == "bold" ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(context, size);

        cairo_font_extents_t metrics;
        cairo_font_extents(context, &metrics);

        auto value = static_cast<float>(metrics.height);

        cairo_destroy(context);
        cairo_surface_destroy(surface);

        return value;
    }

}