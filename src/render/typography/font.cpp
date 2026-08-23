#include "typography/font.hpp"
#include "logger.hpp"

#include <harfbuzz/hb-ot.h>
#include <utility>

namespace render::typography {

    Font::~Font() noexcept {
        dispose();
    }

    Font::Font(Font&& input) noexcept
        : parent(std::exchange(input.parent, nullptr)),
          handle(std::exchange(input.handle, nullptr)),
          points(std::exchange(input.points, 0.0f)) {}

    Font& Font::operator=(Font&& input) noexcept {
        if (this != &input) {
            dispose();
            parent = std::exchange(input.parent, nullptr);
            handle = std::exchange(input.handle, nullptr);
            points = std::exchange(input.points, 0.0f);
        }
        return *this;
    }

    bool Font::compose(const Face& face, const float size) noexcept {
        if (!face.hb()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Invalid face instance for font composition");
            return false;
        }
        dispose();

        handle = hb_font_create(face.hb());
        if (!handle) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "HarfBuzz font creation failed");
            return false;
        }

        const int scale = static_cast<int>(size * 64.0f);
        hb_font_set_scale(handle, scale, scale);
        hb_ot_font_set_funcs(handle);

        parent = &face;
        points = size;
        return true;
    }

    void Font::dispose() noexcept {
        if (handle) {
            hb_font_destroy(handle);
            handle = nullptr;
        }
        parent = nullptr;
        points = 0.0f;
    }

    Font::Metric Font::metrics(const float scale) const noexcept {
        if (!handle || scale == 0.0f) return {};
        hb_font_extents_t extents{};
        if (hb_font_get_h_extents(handle, &extents)) {
            const float ratio = 1.0f / scale;
            return Metric{
                .ascent = static_cast<float>(extents.ascender) * ratio,
                .descent = static_cast<float>(extents.descender) * ratio,
                .gap = static_cast<float>(extents.line_gap) * ratio,
                .height = static_cast<float>(extents.ascender - extents.descender) * ratio,
                .units = static_cast<float>(parent ? parent->units() : 0),
                .size = points
            };
        }
        return {};
    }

    Font::Box Font::bounds(const std::uint32_t glyph, const float scale) const noexcept {
        if (!handle || scale == 0.0f) return {};
        hb_glyph_extents_t extents{};
        if (hb_font_get_glyph_extents(handle, glyph, &extents)) {
            const float ratio = 1.0f / scale;
            return Box{
                .x = static_cast<float>(extents.x_bearing) * ratio,
                .y = static_cast<float>(extents.y_bearing) * ratio,
                .width = static_cast<float>(extents.width) * ratio,
                .height = static_cast<float>(extents.height) * ratio
            };
        }
        return {};
    }

}