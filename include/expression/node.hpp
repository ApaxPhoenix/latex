#pragma once

#include "memory/slice.hpp"
#include "unicodes.hpp"

#include <cstdint>
#include <string_view>

namespace expression {

    enum class Style : std::uint8_t {
        Inline, // In-text embedded math mode (\(... \), $...$)
        Display // Standalone block-level equation (\[... \], $$...$$)
    };

    struct Node {
        enum class Type : std::uint8_t {
            Variable, // Identifiers, numbers, and basic symbols (e.g., x, 2)
            Binary,   // Infix operators with left/right operands (e.g., +, -)
            Unary,    // Single-operand prefix or postfix operators (e.g., -x, n!)
            Group,    // Delimited sub-expressions (e.g., {...}, (...))
            // Thought of separating these into two different categories: subscripts and superscripts
            // But realized that it will get more confusing on the long run
            // Because of the design language x_1^2 it could be referenced to as x: - sub: 1, sup: 2
            SubSup,   // Base node with attached sub/superscripts (e.g., x_i^2)
            Fraction, // Numerator and denominator pairs (e.g., \frac{a}{b})
            Radical,  // Square or n-th root expressions (e.g., \sqrt[n]{x})
            Accent,   // Diacritics placed over symbols (e.g., \hat{x}, \vec{v})
            Sequence  // Horizontal list of sequential nodes (Math List)
        };

        Type type = Type::Variable;
        Unicodes::Category category = Unicodes::Category::Ordinary;
        Style style = Style::Inline;
        std::uint32_t codepoint = 0;
        std::string_view value{};

        Node* left = nullptr;
        Node* right = nullptr;
        Node* subscript = nullptr;
        Node* superscript = nullptr;
        memory::Slice<Node*> arguments{};
    };

}