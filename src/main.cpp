#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "lexer.hpp"

std::string token_type_to_string(const core::Type type) {
    switch (type) {
        case core::Type::LEFT_BRACE_TOKEN:        return "LEFT_BRACE_TOKEN";
        case core::Type::RIGHT_BRACE_TOKEN:       return "RIGHT_BRACE_TOKEN";
        case core::Type::LEFT_BRACKET_TOKEN:      return "LEFT_BRACKET_TOKEN";
        case core::Type::RIGHT_BRACKET_TOKEN:     return "RIGHT_BRACKET_TOKEN";
        case core::Type::LEFT_PARENTHESIS_TOKEN:  return "LEFT_PARENTHESIS_TOKEN";
        case core::Type::RIGHT_PARENTHESIS_TOKEN: return "RIGHT_PARENTHESIS_TOKEN";
        case core::Type::DOLLAR_TOKEN:            return "DOLLAR_TOKEN";
        case core::Type::DOUBLE_DOLLAR_TOKEN:     return "DOUBLE_DOLLAR_TOKEN";
        case core::Type::AMPERSAND_TOKEN:         return "AMPERSAND_TOKEN";
        case core::Type::ROW_BREAK_TOKEN:         return "ROW_BREAK_TOKEN";
        case core::Type::UNDERSCORE_TOKEN:        return "UNDERSCORE_TOKEN";
        case core::Type::CARET_TOKEN:             return "CARET_TOKEN";
        case core::Type::TILDE_TOKEN:             return "TILDE_TOKEN";
        case core::Type::COMMENT_TOKEN:           return "COMMENT_TOKEN";
        case core::Type::PLUS_TOKEN:              return "PLUS_TOKEN";
        case core::Type::MINUS_TOKEN:             return "MINUS_TOKEN";
        case core::Type::ASTERISK_TOKEN:          return "ASTERISK_TOKEN";
        case core::Type::SLASH_TOKEN:             return "SLASH_TOKEN";
        case core::Type::EQUAL_TOKEN:             return "EQUAL_TOKEN";
        case core::Type::COMMA_TOKEN:             return "COMMA_TOKEN";
        case core::Type::COMMAND_TOKEN:           return "COMMAND_TOKEN";
        case core::Type::IDENTIFIER_TOKEN:        return "IDENTIFIER_TOKEN";
        case core::Type::INTEGER_TOKEN:           return "INTEGER_TOKEN";
        case core::Type::FLOAT_TOKEN:             return "FLOAT_TOKEN";
        case core::Type::TEXT_TOKEN:              return "TEXT_TOKEN";
        case core::Type::ESCAPE_TOKEN:            return "ESCAPE_TOKEN";
        case core::Type::NEWLINE_TOKEN:           return "NEWLINE_TOKEN";
        case core::Type::WHITESPACE_TOKEN:        return "WHITESPACE_TOKEN";
        case core::Type::EOF_TOKEN:               return "EOF_TOKEN";
        default:                                  return "UNKNOWN";
    }
}

int main() {
    std::ifstream file("../tests/main.tex");
    if (!file.is_open()) {
        std::cerr << "error: failed to open source file '../tests/main.tex'\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    core::Lexer lexer(source);

    for (std::vector<core::Token> tokens = lexer.tokenize(); const auto&[type, value, line, column] : tokens) {
        std::cout << "token: type=" << token_type_to_string(type)
                  << " line=" << line
                  << " col=" << column
                  << " val=\"" << value << "\"\n";
    }

    return 0;
}