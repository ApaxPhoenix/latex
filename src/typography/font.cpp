#include "typography/font.hpp"

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>
#include <include/core/SkFontMgr.h>

#if defined(_WIN32)
#include <include/ports/SkTypeface_win.h>
#elif defined(__APPLE__)
#include <include/ports/SkFontMgr_mac_ct.h>
#elif defined(__linux__) || defined(__ANDROID__)
#include <include/ports/SkFontMgr_fontconfig.h>
#endif

namespace typography {

    FT_Library Font::engine = nullptr;
    std::uint32_t Font::references = 0;

    Font::Font() {
        if (references == 0) {
            if (FT_Init_FreeType(&engine) != 0) {
                engine = nullptr;
            }
        }
        if (engine) {
            ++references;
        }
    }

    Font::~Font() {
        dispose();
        if (references > 0) {
            --references;
            if (references == 0 && engine) {
                FT_Done_FreeType(engine);
                engine = nullptr;
            }
        }
    }

    Font::Font(Font&& input) noexcept
        : face(input.face),
          handle(input.handle),
          location(std::move(input.location)) {
        input.face = nullptr;
        input.handle = nullptr;
    }

    Font& Font::operator=(Font&& input) noexcept {
        if (this != &input) {
            dispose();
            face = input.face;
            handle = input.handle;
            location = std::move(input.location);
            input.face = nullptr;
            input.handle = nullptr;
        }
        return *this;
    }

    bool Font::load(const std::string_view path, const unsigned int size) {
        if (!engine) return false;

        dispose();
        location = std::string(path);

        if (FT_New_Face(engine, location.c_str(), 0, &face) != 0) return false;
        if (FT_Set_Pixel_Sizes(face, 0, size) != 0) return false;

        handle = hb_ft_font_create_referenced(face);
        return handle != nullptr;
    }

    Shape Font::shape(memory::Arena& arena, const std::string_view text) const {
        if (!handle || text.empty()) {
            return {};
        }

        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
        hb_buffer_guess_segment_properties(buffer);

        hb_shape(handle, buffer, nullptr, 0);

        unsigned int count = 0;
        hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &count);
        hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &count);

        if (count == 0) {
            hb_buffer_destroy(buffer);
            return {};
        }

        memory::Slice<std::uint32_t> glyphs = arena.allocate<std::uint32_t>(count);
        memory::Slice<Position> positions = arena.allocate<Position>(count);
        memory::Slice<float> advances = arena.allocate<float>(count);

        float accumulate = 0.0f;

        for (unsigned int index = 0; index < count; ++index) {
            glyphs[index] = info[index].codepoint;

            const float x = static_cast<float>(pos[index].x_offset) / 64.0f;
            const float y = static_cast<float>(pos[index].y_offset) / 64.0f;
            const float dx = static_cast<float>(pos[index].x_advance) / 64.0f;

            positions[index] = Position{x, y};
            advances[index] = dx;
            accumulate += dx;
        }

        hb_buffer_destroy(buffer);

        return Shape{glyphs, positions, advances, accumulate};
    }

    Metrics Font::metrics() const noexcept {
        if (!handle) return {};
        hb_font_extents_t extents{};
        if (hb_font_get_h_extents(handle, &extents)) {
            hb_face_t* source = hb_font_get_face(handle);
            return Metrics{
                static_cast<float>(extents.ascender) / 64.0f,
                static_cast<float>(extents.descender) / 64.0f,
                static_cast<float>(extents.line_gap) / 64.0f,
                static_cast<float>(hb_face_get_upem(source))
            };
        }
        return {};
    }

    Box Font::bounds(const std::uint32_t glyph) const noexcept {
        if (!handle) return {};
        hb_glyph_extents_t extents{};
        if (hb_font_get_glyph_extents(handle, glyph, &extents)) {
            return Box{
                static_cast<float>(extents.x_bearing) / 64.0f,
                static_cast<float>(extents.y_bearing) / 64.0f,
                static_cast<float>(extents.width) / 64.0f,
                static_cast<float>(extents.height) / 64.0f
            };
        }
        return {};
    }

    float Font::constant(const unsigned int target) const noexcept {
        if (!handle) return 0.0f;
        const auto key = static_cast<hb_ot_math_constant_t>(target);
        return static_cast<float>(hb_ot_math_get_constant(handle, key)) / 64.0f;
    }

    sk_sp<SkTypeface> Font::typeface() const {
        if (location.empty()) return nullptr;

        sk_sp<SkFontMgr> manager;

        #if defined(_WIN32)
                manager = SkFontMgr_New_DirectWrite();
        #elif defined(__APPLE__)
                manager = SkFontMgr_New_CoreText(nullptr);
        #elif defined(__linux__) || defined(__ANDROID__)
                manager = SkFontMgr_New_Fontconfig(nullptr);
        #endif

        if (manager) {
            return manager->makeFromFile(location.c_str());
        }

        return nullptr;
    }

    void Font::dispose() noexcept {
        if (handle) { hb_font_destroy(handle); handle = nullptr; }
        if (face) { FT_Done_Face(face); face = nullptr; }
        location.clear();
    }

}