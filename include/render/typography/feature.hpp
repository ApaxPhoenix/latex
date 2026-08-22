#pragma once

#include <hb.h>
#include <string_view>

namespace typography {

    struct Feature {
        hb_feature_t raw{};

        constexpr Feature() noexcept = default;

        explicit constexpr Feature(const std::string_view tag, const std::uint32_t value = 1) noexcept {
            char buffer[4] = {' ', ' ', ' ', ' '};
            for (std::size_t i = 0; i < tag.size() && i < 4; ++i) {
                buffer[i] = tag[i];
            }
            raw.tag = HB_TAG(buffer[0], buffer[1], buffer[2], buffer[3]);
            raw.value = value;
            raw.start = HB_FEATURE_GLOBAL_START;
            raw.end = HB_FEATURE_GLOBAL_END;
        }
    };

}