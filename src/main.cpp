#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string_view>
#include "lexer.hpp"
#include "parser.hpp"
#include "arena.hpp"
#include "ast.hpp"

std::string_view token_type_to_string(const core::lexer::Type type) {
    static const std::unordered_map<core::lexer::Type, std::string_view> type_map = {
        {core::lexer::Type::LEFT_BRACE,        "LEFT_BRACE"},
        {core::lexer::Type::RIGHT_BRACE,       "RIGHT_BRACE"},
        {core::lexer::Type::LEFT_BRACKET,      "LEFT_BRACKET"},
        {core::lexer::Type::RIGHT_BRACKET,     "RIGHT_BRACKET"},
        {core::lexer::Type::LEFT_PARENTHESIS,  "LEFT_PARENTHESIS"},
        {core::lexer::Type::RIGHT_PARENTHESIS, "RIGHT_PARENTHESIS"},
        {core::lexer::Type::ROW_BREAK,         "ROW_BREAK"},
        {core::lexer::Type::DOLLAR,            "DOLLAR"},
        {core::lexer::Type::AMPERSAND,         "AMPERSAND"},
        {core::lexer::Type::UNDERSCORE,        "UNDERSCORE"},
        {core::lexer::Type::CARET,             "CARET"},
        {core::lexer::Type::TILDE,             "TILDE"},
        {core::lexer::Type::COMMENT,           "COMMENT"},
        {core::lexer::Type::COMMAND,           "COMMAND"},
        {core::lexer::Type::IDENTIFIER,        "IDENTIFIER"},
        {core::lexer::Type::PROSE,             "PROSE"},
        {core::lexer::Type::NEWLINE,           "NEWLINE"},
        {core::lexer::Type::WHITESPACE,        "WHITESPACE"},
        {core::lexer::Type::END_OF_FILE,       "END_OF_FILE"}
    };

    if (const auto it = type_map.find(type); it != type_map.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

std::string_view ast_type_to_string(const core::ast::Type type) {
    static const std::unordered_map<core::ast::Type, std::string_view> type_map = {
        {core::ast::Type::SCOPE,   "SCOPE"},
        {core::ast::Type::COMMAND, "COMMAND"},
        {core::ast::Type::TEXT,    "TEXT"}
    };

    if (auto it = type_map.find(type); it != type_map.end()) {
        return it->second;
    }
    return "UNKNOWN";
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

    core::lexer::Lexer lexer(source);
    std::vector<core::lexer::Token> tokens = lexer.tokenize();

    for (const auto&[type, value, line, column] : tokens) {
        std::cout << "token: type=" << token_type_to_string(type)
                  << " line=" << line
                  << " col=" << column
                  << " val=\"" << value << "\"\n";
    }

    core::lexer::Cursor stream(tokens);
    core::arena::Arena pool;
    core::parser::Parser parser(stream, pool);
    std::vector<core::ast::Node*> nodes = parser.parse();

    for (const auto* node : nodes) {
        std::cout << "node: type=" << ast_type_to_string(node->type)
                  << " depth=" << node->depth
                  << " val=\"" << node->value << "\"\n";
    }

    return 0;
}