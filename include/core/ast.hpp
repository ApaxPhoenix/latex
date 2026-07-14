#pragma once

#include <vector>
#include <string_view>

namespace core::ast {

    enum class Type {
        DOCUMENT,
        TEXT,
        IMAGE,
        MACRO,
        SCOPE,
        BLOCK,
        ENVIRONMENT,
        VERBATIM,
        PARAGRAPH,
        MATH_INLINE,
        MATH_DISPLAY,
        OPERATOR,
        SUBSCRIPT,
        SUPERSCRIPT,
        ALIGNMENT_MARKER,
        ROW_BREAK_MARKER
    };

    struct Node {
        Type type;
        std::string_view value;
        std::vector<Node*> nodes;
    };

}