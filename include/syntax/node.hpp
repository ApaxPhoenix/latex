#pragma once

#include "memory/location.hpp"
#include "memory/slice.hpp"

#include <string_view>

namespace syntax::expression { struct Node; }

namespace syntax {

    struct Node {
        enum class Type {
            Text,        // Plain unformatted character stream
            Paragraph,   // Explicit paragraph break (\par)
            Expression,  // Math block entry point (\(...\), \[...\], $...$)
            Group,       // Enclosed scope block ({...})
            Environment, // Generalized environment (\begin{name}...\end{name})
            Verbatim,    // Raw unformatted code container
            Macro,       // Macro expansion or directive (\def, \newcommand)
            Argument,    // Positional or optional argument payload ({...}, [...])
            Alignment,   // Table or grid cell/row alignment marker (&, \\)
            Comment,     // Source code line comment (% ...)
            Directive    // Opaque engine-produced payload (boxes, glue, rules, penalties)
        };

        Type type = Type::Text;
        std::string_view value{};
        memory::Location location{};
        memory::Slice<Node*> nodes{};
        const expression::Node* expression{nullptr};
        void* directive{nullptr};

        Node() = default;

        Node(const Type type, const std::string_view value, const memory::Location location, const memory::Slice<Node*> nodes = {}, const bool display = false)
            : type(type), value(value), location(location), nodes(nodes) {}
    };

}