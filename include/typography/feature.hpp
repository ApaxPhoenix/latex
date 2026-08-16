#pragma once

#include <hb.h>
#include <string_view>

namespace typography {

    struct Feature {
        hb_feature_t raw{};

        explicit constexpr Feature(const std::string_view tag, const std::uint32_t value = 1) noexcept {
            raw.tag = HB_TAG(tag[0], tag[1], tag[2], tag[3]);
            raw.value = value;
            raw.start = HB_FEATURE_GLOBAL_START;
            raw.end = HB_FEATURE_GLOBAL_END;
        }
    };

}