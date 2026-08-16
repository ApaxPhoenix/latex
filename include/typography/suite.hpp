#pragma once

#include "typography/font.hpp"
#include "typography/fontconfig.hpp"

#include <array>
#include <string_view>

namespace typography {

    class Suite {
    public:
        enum class Face : std::size_t {
            Regular = 0,
            Bold = 1,
            Italic = 2,
            Mono = 3,
            Count = 4
        };

        Suite() = default;
        ~Suite() = default;

        Suite(const Suite&) = delete;
        Suite& operator=(const Suite&) = delete;
        Suite(Suite&&) noexcept = default;
        Suite& operator=(Suite&&) noexcept = default;

        bool load(Face face, std::string_view path, unsigned int size);

        bool load(Face face, FontConfig& configuration, std::string_view pattern, unsigned int size);

        [[nodiscard]] const Font* fetch(Face face) const noexcept;
        [[nodiscard]] Font* fetch(Face face) noexcept;
        [[nodiscard]] sk_sp<SkTypeface> typeface(Face face) const;

    private:
        std::array<Font, static_cast<std::size_t>(Face::Count)> faces{};
    };

}