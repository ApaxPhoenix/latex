#pragma once

#include "core/ast.hpp"
#include "core/arena.hpp"
#include "pipeline/font.hpp"
#include "pipeline/raster.hpp"

#include <vector>
#include <optional>
#include <string>

namespace pipeline {

    enum class Display { FLEX, INLINE };
    enum class Direction { ROW, COLUMN };

    struct Properties {
        Display display = Display::FLEX;
        Direction direction = Direction::ROW;
        std::string family = "latin";
        std::string variant = "regular";
        float height = 0.0f;
        float width = 0.0f;
        float size = 12.0f;
    };

    struct Widget {
        std::optional<Properties> properties;
        std::string_view text;
        std::string asset;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        std::vector<Widget*> children;
    };

    class Layout {
        core::arena::Arena& arena;

    public:
        explicit Layout(core::arena::Arena& arena);
        Widget* compute(const core::ast::Node* node, const Font& font, const Raster& raster) const;
    };

}