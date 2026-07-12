#include "parser.hpp"
#include <iostream>

namespace core::parser {

    Parser::Parser(lexer::Cursor stream, arena::Arena& pool)
        : cursor(std::move(stream)), arena(pool) {}

    std::vector<ast::Node*> Parser::parse() {
        std::vector<ast::Node*> nodes;

        while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
            lexer::Token tokens = cursor.lookahead();

            if (tokens.type == lexer::Type::LEFT_BRACE) {
                cursor.advance();
                auto* node = arena.construct<ast::Node>(ast::Type::SCOPE, depth, tokens.value);
                depth++;
                nodes.push_back(node);
                continue;
            }

            if (tokens.type == lexer::Type::RIGHT_BRACE) {
                cursor.advance();
                if (depth > 0) {
                    depth--;
                }
                continue;
            }

            if (tokens.type == lexer::Type::COMMAND) {
                cursor.advance();
                auto* node = arena.construct<ast::Node>(ast::Type::COMMAND, depth, tokens.value);
                nodes.push_back(node);
                continue;
            }

            if (tokens.type == lexer::Type::PROSE || tokens.type == lexer::Type::IDENTIFIER) {
                cursor.advance();
                auto* node = arena.construct<ast::Node>(ast::Type::TEXT, depth, tokens.value);
                nodes.push_back(node);
                continue;
            }

            cursor.advance();
        }

        return nodes;
    }

}