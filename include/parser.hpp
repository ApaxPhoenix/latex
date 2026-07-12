#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include "arena.hpp"

#include <vector>

namespace core::parser {
    class Parser {
    public:
        explicit Parser(lexer::Cursor cursor, arena::Arena& arena);

        std::vector<ast::Node*> parse();

    private:
        lexer::Cursor cursor;
        arena::Arena& arena;
        uint16_t depth = 0;
    };
}