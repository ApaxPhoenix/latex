#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "typography/feature.hpp"
#include "typography/font.hpp"

#include <hb.h>
#include <string_view>

namespace typography {

    class Shaper {
    public:
        explicit Shaper(memory::Arena& arena) noexcept;

        [[nodiscard]] memory::Slice<layout::Node*> shape(
            const Font& font,
            std::string_view text
        ) const;

        [[nodiscard]] memory::Slice<layout::Node*> shape(
            const Font& font,
            std::string_view text,
            memory::Slice<Feature> features
        ) const;

    private:
        memory::Arena& arena;
    };

}