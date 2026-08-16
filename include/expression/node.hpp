#pragma once

#include "memory/slice.hpp"
#include "unicodes.hpp"

#include <cstdint>
#include <string_view>

namespace expression {

    struct Node {
        enum class Type : std::uint8_t {
            Variable,
            Binary,
            Unary,
            Group,
            Subscript,
            Superscript,
            Fraction,
            Radical,
            Macro
        };

        Type type = Type::Variable;
        Unicodes::Type type_ = Unicodes::Type::Ordinary;
        std::uint32_t codepoint = 0;
        std::string_view value{};

        Node* left = nullptr;
        Node* right = nullptr;
        memory::Slice<Node*> arguments{};
    };

}