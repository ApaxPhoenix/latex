#pragma once

#include <harfbuzz/hb.h>
#include <string_view>

namespace render::typography {

    struct Feature {
        hb_feature_t raw{};

        constexpr Feature() noexcept = default;

        explicit constexpr Feature(const std::string_view tag, const std::uint32_t value = 1) noexcept {
            char buffer[4] = {' ', ' ', ' ', ' '};
            for (std::size_t index = 0; index < tag.size() && index < 4; ++index) {
                buffer[index] = tag[index];
            }
            raw.tag = HB_TAG(buffer[0], buffer[1], buffer[2], buffer[3]);
            raw.value = value;
            raw.start = HB_FEATURE_GLOBAL_START;
            raw.end = HB_FEATURE_GLOBAL_END;
        }
    };

}