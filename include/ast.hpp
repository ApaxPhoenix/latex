#pragma once

#include <vector>
#include <string_view>

namespace core::ast {

    enum class Type {
        DOCUMENT,          // Root container node for the entire file stream
        TEXT,              // Plain prose sequences or atomic mathematical symbols ("Hello", "x", "42")
        COMMAND,           // Control sequences / macro keywords ("\section", "\vec", "\frac")
        SCOPE,             // A structural brace-enclosed grouping boundary ({ ... })
        BLOCK,             // Isolated structural container layout bound by alignment markers (\begin{matrix} ... \end{matrix})

        MATH_INLINE,       // Inline mathematical expression flow embedded directly within text streams ($ ... $)
        MATH_DISPLAY,      // Centered block mathematical layout isolated from surrounding text (\[ ... \])
        OPERATOR,          // Mathematical binary/relation operators with structural precedence weights (+, -, *, /, =)
        SUBSCRIPT,         // Downward baseline shift and font scaling for subscript layouts (_)
        SUPERSCRIPT,       // Upward baseline shift and font scaling for superscript layouts (^)

        ALIGNMENT_MARKER,  // Flat layout-agnostic delimiter tracking column alignment separation (&)
        ROW_BREAK_MARKER   // Flat layout-agnostic delimiter tracking explicit row termination (\\)
    };

    struct Node {
        Type type;
        uint16_t depth;
        std::string_view value;
    };

}