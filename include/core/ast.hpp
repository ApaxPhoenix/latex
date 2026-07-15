#pragma once

#include <vector>
#include <string_view>

namespace core::ast {

    enum class Unit {
        PT,  // Point
        PC,  // Pica
        IN,  // Inch
        BP,  // Big Point
        CM,  // Centimeter
        MM,  // Millimeter
        DD,  // Didot
        CC,  // Cicero
        SP,  // Scaled Point
        EM,  // Relative Width
        EX   // Relative Height
    };

    enum class Type {
        DOCUMENT,       // Root node of the file
        PARAGRAPH,      // Text separated by empty lines
        TEXT,           // Raw character data
        MACRO,          // Control sequence (e.g., \section)
        GROUP,          // Braces {...} or environments
        VERBATIM,       // Unparsed raw text (e.g., code blocks)
        INLINE,         // Inline math: \( ... \) or $ ... $
        DISPLAY,        // Display math: \[ ... \] or $$ ... $$
        SUBSCRIPT,      // Subscript: _
        SUPERSCRIPT,    // Superscript: ^
        ALIGN,          // Alignment anchor: &
        BREAK           /* Row split: \\ */
    };

    struct Node {
        Type type;
        std::string_view value;
        std::vector<Node*> nodes;
    };

}