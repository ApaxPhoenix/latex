#pragma once

#include "memory/arena.hpp"

#include <include/core/SkTypeface.h>
#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string_view>

namespace typography {

    struct Position {
        float x{0.0f};
        float y{0.0f};
    };

    struct Box {
        float x{0.0f};
        float y{0.0f};
        float width{0.0f};
        float height{0.0f};
    };

    struct Metrics {
        float ascent{0.0f};
        float descent{0.0f};
        float height{0.0f};
        float units{0.0f};
    };

    struct Shape {
        memory::Slice<std::uint32_t> glyphs{};
        memory::Slice<Position> positions{};
        memory::Slice<float> advances{};
        float width{0.0f};
    };

    class Font {
    public:
        Font();
        ~Font();

        Font(Font&& input) noexcept;
        Font& operator=(Font&& input) noexcept;

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        bool load(std::string_view path, unsigned int size);
        void dispose() noexcept;

        [[nodiscard]] Shape shape(memory::Arena& arena, std::string_view text) const;
        [[nodiscard]] Metrics metrics() const noexcept;
        [[nodiscard]] Box bounds(std::uint32_t glyph) const noexcept;
        [[nodiscard]] float constant(unsigned int target) const noexcept;

        [[nodiscard]] hb_font_t* fetch() const noexcept { return handle; }
        [[nodiscard]] sk_sp<SkTypeface> typeface() const;

    private:
        FT_Face face{};
        hb_font_t* handle{};
        std::string location{};

        static FT_Library engine;
        static std::uint32_t references;
    };

}