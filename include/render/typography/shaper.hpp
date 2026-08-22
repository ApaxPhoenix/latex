#pragma once

#include "typography/font.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "render/layout/node.hpp"

#include <harfbuzz/hb.h>
#include <string_view>

namespace typography {

    class Shaper {
    public:
        struct Feature {
            hb_feature_t raw{};
        };

        explicit Shaper(memory::Arena& arena) noexcept : arena(arena) {}

        [[nodiscard]] memory::Slice<render::layout::Node*> shape(
            memory::Slice<const Font*> fonts,
            std::string_view text,
            memory::Slice<Feature> features
        ) const;

    private:
        memory::Arena& arena;
    };

}