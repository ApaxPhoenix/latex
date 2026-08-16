#pragma once

#include "memory/location.hpp"

#include <string>
#include <string_view>

namespace syntax {

    class Traceback {
    public:
        enum class Type : std::uint8_t {
            Group,       // Mismatched or unclosed curly brace scope group
            Equation,    // Invalid or unclosed text formula boundary
            Environment, // Unmatched \begin and \end environment block
            Delimiter,   // Unbalanced \left and \right delimiter pair
            Argument,    // Missing or malformed macro argument
            Token,       // Unexpected or invalid token encountered
            End,         // Premature end of file reached
            Macro,       // Undefined or invalid macro command
            Recursion,   // Infinite macro expansion recursion detected
            Memory,      // Memory arena capacity limit exceeded
            Catcode,     // Invalid character category code assignment
            Scope,       // Unmatched scope exit operation
            Primitive,   // Failure executing underlying compiler primitive
            Dimension,   // Invalid or unparseable unit dimension specification
            Register,    // Out-of-bounds register index access
            Syntax       // General parsing syntax rule violation
        };

        Traceback() = default;

        constexpr Traceback(const Type type, const memory::Location location, const std::string_view message) noexcept
            : type_(type), location_(location), message_(message) {}

        [[nodiscard]] constexpr Type type() const noexcept { return type_; }
        [[nodiscard]] constexpr const memory::Location& location() const noexcept { return location_; }
        [[nodiscard]] const std::string& message() const noexcept { return message_; }
        [[nodiscard]] std::string format() const;

    private:
        Type type_ = Type::Syntax;
        memory::Location location_{};
        std::string message_{};
    };

}