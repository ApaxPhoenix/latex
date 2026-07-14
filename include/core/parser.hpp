#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include "arena.hpp"

#include <vector>
#include <unordered_map>

namespace core::parser {

    class Parser {
    public:
        explicit Parser(lexer::Cursor cursor, arena::Arena& arena);
        std::vector<ast::Node*> parse();

    private:
        lexer::Cursor cursor;
        arena::Arena& arena;
        std::unordered_map<std::string_view, std::vector<lexer::Token>> macros;

        std::vector<ast::Node*> query(lexer::Type closer);
        ast::Node* node();
    };

}