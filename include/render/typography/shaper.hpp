#pragma once

#include "typography/font.hpp"
#include "typography/feature.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "render/layout/node.hpp"

#include <string_view>

namespace render::typography {

    class Shaper {
    public:
        explicit Shaper(memory::Arena& arena) noexcept : arena(arena) {}

        [[nodiscard]] memory::Slice<layout::Node*> shape(
            memory::Slice<const Font*> fonts,
            std::string_view text,
            memory::Slice<Feature> features
        ) const;

    private:
        struct Utf {
            std::uint32_t code{0};
            std::size_t size{0};
        };

        [[nodiscard]] static constexpr Utf resolve(std::string_view text, std::size_t index) noexcept;

        memory::Arena& arena;
    };

}