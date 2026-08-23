#pragma once

#include "typography/face.hpp"

#include <harfbuzz/hb.h>
#include <cstdint>

namespace render::typography {

    class Font {
    public:
        struct Metric {
            float ascent{0.0f};
            float descent{0.0f};
            float gap{0.0f};
            float height{0.0f};
            float units{0.0f};
            float size{0.0f};
        };

        struct Box {
            float x{0.0f};
            float y{0.0f};
            float width{0.0f};
            float height{0.0f};
        };

        Font() noexcept = default;
        ~Font() noexcept;

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;
        Font(Font&& input) noexcept;
        Font& operator=(Font&& input) noexcept;

        [[nodiscard]] bool compose(const Face& face, float size) noexcept;
        void dispose() noexcept;

        [[nodiscard]] Metric metrics(float scale = 64.0f) const noexcept;
        [[nodiscard]] Box bounds(std::uint32_t glyph, float scale = 64.0f) const noexcept;

        [[nodiscard]] hb_font_t* hb() const noexcept { return handle; }
        [[nodiscard]] const Face* face() const noexcept { return parent; }
        [[nodiscard]] float size() const noexcept { return points; }

    private:
        const Face* parent{nullptr};
        hb_font_t* handle{nullptr};
        float points{0.0f};
    };

}