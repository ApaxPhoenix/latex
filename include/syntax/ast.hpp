#pragma once

#include "memory/location.hpp"
#include "memory/slice.hpp"

#include <string_view>

namespace syntax {

    struct Node {
        enum class Type {
            Text,        // Plain unformatted character equations stream
            Paragraph,   // Paragraph break control node (\par)
            Equation,    // Inline or display text block (\(... \) or \[... \])
            Group,       // Enclosed local scope block ({...})
            Document,    // Main document environment (\begin{document}...\end{document})
            Verbatim,    // Raw unformatted code container (\begin{verbatim}...\end{verbatim})
            Macro        // Macro expansion or definition directive (\def)
        };

        Type type = Type::Text;
        std::string_view value{};
        memory::Location location{};
        memory::Slice<Node*> nodes{};

        Node() = default;

        Node(const Type type, const std::string_view value, const memory::Location location, const memory::Slice<Node*> nodes = {})
            : type(type), value(value), location(location), nodes(nodes) {}
    };

}