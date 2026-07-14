#include "core/parser.hpp"
#include <charconv>

namespace core::parser {

    Parser::Parser(lexer::Cursor cursor, arena::Arena& arena)
        : cursor(std::move(cursor)), arena(arena) {}

    std::vector<ast::Node*> Parser::parse() {
        std::vector<ast::Node*> nodes;

        while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
            if (cursor.lookahead().type == lexer::Type::COMMENT) {
                cursor.advance();
                if (cursor.lookahead().type == lexer::Type::NEWLINE) {
                    cursor.advance();
                }
                continue;
            }

            if (cursor.lookahead().type == lexer::Type::WHITESPACE || cursor.lookahead().type == lexer::Type::NEWLINE) {
                size_t lines = 0;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE || cursor.lookahead().type == lexer::Type::NEWLINE) {
                    if (cursor.advance().type == lexer::Type::NEWLINE) {
                        lines++;
                    }
                }
                if (lines > 1) {
                    nodes.push_back(arena.construct<ast::Node>(ast::Type::PARAGRAPH, "\\par"));
                } else {
                    if (nodes.empty() || nodes.back()->type != ast::Type::TEXT || nodes.back()->value != " ") {
                        nodes.push_back(arena.construct<ast::Node>(ast::Type::TEXT, " "));
                    }
                }
                continue;
            }

            if (cursor.lookahead().type == lexer::Type::UNDERSCORE || cursor.lookahead().type == lexer::Type::CARET) {
                auto* node = arena.construct<ast::Node>(
                    (cursor.lookahead().type == lexer::Type::UNDERSCORE) ? ast::Type::SUBSCRIPT : ast::Type::SUPERSCRIPT, 
                    cursor.advance().value
                );

                if (auto* child = this->node()) {
                    node->nodes.push_back(child);
                }

                if (!nodes.empty()) {
                    nodes.back()->nodes.push_back(node);
                } else {
                    nodes.push_back(node);
                }
                continue;
            }

            if (auto* node = this->node()) {
                nodes.push_back(node);
            }
        }
        return nodes;
    }

    std::vector<ast::Node*> Parser::query(const lexer::Type closer) {
        std::vector<ast::Node*> list;

        while (cursor.lookahead().type != lexer::Type::END_OF_FILE && cursor.lookahead().type != closer) {
            if (cursor.lookahead().type == lexer::Type::COMMENT) {
                cursor.advance();
                if (cursor.lookahead().type == lexer::Type::NEWLINE) {
                    cursor.advance();
                }
                continue;
            }

            if (cursor.lookahead().type == lexer::Type::WHITESPACE || cursor.lookahead().type == lexer::Type::NEWLINE) {
                size_t lines = 0;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE || cursor.lookahead().type == lexer::Type::NEWLINE) {
                    if (cursor.advance().type == lexer::Type::NEWLINE) {
                        lines++;
                    }
                }
                if (lines > 1) {
                    list.push_back(arena.construct<ast::Node>(ast::Type::PARAGRAPH, "\\par"));
                } else {
                    if (list.empty() || list.back()->type != ast::Type::TEXT || list.back()->value != " ") {
                        list.push_back(arena.construct<ast::Node>(ast::Type::TEXT, " "));
                    }
                }
                continue;
            }

            if (cursor.lookahead().type == lexer::Type::UNDERSCORE || cursor.lookahead().type == lexer::Type::CARET) {
                auto* node = arena.construct<ast::Node>(
                    (cursor.lookahead().type == lexer::Type::UNDERSCORE) ? ast::Type::SUBSCRIPT : ast::Type::SUPERSCRIPT,
                    cursor.advance().value
                );

                if (auto* child = this->node()) {
                    node->nodes.push_back(child);
                }

                if (!list.empty()) {
                    list.back()->nodes.push_back(node);
                } else {
                    list.push_back(node);
                }
                continue;
            }

            if (auto* node = this->node()) {
                list.push_back(node);
            }
        }
        return list;
    }

    ast::Node* Parser::node() {
        if (cursor.lookahead().type == lexer::Type::END_OF_FILE) {
            return nullptr;
        }

        if (cursor.lookahead().type == lexer::Type::LEFT_BRACE) {
            auto* node = arena.construct<ast::Node>(ast::Type::SCOPE, cursor.advance().value);
            node->nodes = query(lexer::Type::RIGHT_BRACE);
            if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                cursor.advance();
            }
            return node;
        }

        if (cursor.lookahead().type == lexer::Type::MACRO) {
            if (cursor.lookahead().value == "\\ifx") {
                cursor.advance();
                auto primary = cursor.advance();
                if (auto secondary = cursor.advance(); primary.type != secondary.type || primary.value != secondary.value) {
                    size_t depth = 1;
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE && depth > 0) {
                        if (auto token = cursor.advance(); token.value == "\\ifx" || token.value == "\\ifnum" || token.value == "\\ifdim" || token.value == "\\ifdefined") {
                            depth++;
                        } else if (token.value == "\\fi") {
                            if (depth == 1) break;
                            depth--;
                        } else if (token.value == "\\else") {
                            if (depth == 1) break;
                        }
                    }
                }
                return nullptr;
            }

            if (cursor.lookahead().value == "\\ifnum") {
                cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto primary = cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto comparison = cursor.advance().value;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto secondary = cursor.advance();

                int first = 0, second = 0;
                std::from_chars(primary.value.data(), primary.value.data() + primary.value.size(), first);
                std::from_chars(secondary.value.data(), secondary.value.data() + secondary.value.size(), second);

                if ((comparison == "<" && first >= second) || (comparison == ">" && first <= second) || (comparison == "=" && first != second)) {
                    size_t depth = 1;
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE && depth > 0) {
                        if (auto token = cursor.advance(); token.value == "\\ifx" || token.value == "\\ifnum" || token.value == "\\ifdim" || token.value == "\\ifdefined") {
                            depth++;
                        } else if (token.value == "\\fi") {
                            if (depth == 1) break;
                            depth--;
                        } else if (token.value == "\\else") {
                            if (depth == 1) break;
                        }
                    }
                }
                return nullptr;
            }

            if (cursor.lookahead().value == "\\ifdim") {
                cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto primary = cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto comparison = cursor.advance().value;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto secondary = cursor.advance();

                std::string alpha(primary.value), beta(secondary.value);
                if (auto position = alpha.find_first_not_of("0123456789.-"); position != std::string::npos) alpha = alpha.substr(0, position);
                if (auto offset = beta.find_first_not_of("0123456789.-"); offset != std::string::npos) beta = beta.substr(0, offset);

                if ((comparison == "<" && std::strtod(alpha.c_str(), nullptr) >= std::strtod(beta.c_str(), nullptr)) ||
                    (comparison == ">" && std::strtod(alpha.c_str(), nullptr) <= std::strtod(beta.c_str(), nullptr)) ||
                    (comparison == "=" && std::strtod(alpha.c_str(), nullptr) != std::strtod(beta.c_str(), nullptr))) {
                    size_t depth = 1;
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE && depth > 0) {
                        if (auto token = cursor.advance(); token.value == "\\ifx" || token.value == "\\ifnum" || token.value == "\\ifdim" || token.value == "\\ifdefined") {
                            depth++;
                        } else if (token.value == "\\fi") {
                            if (depth == 1) break;
                            depth--;
                        } else if (token.value == "\\else") {
                            if (depth == 1) break;
                        }
                    }
                }
                return nullptr;
            }

            if (cursor.lookahead().value == "\\ifdefined") {
                cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                if (!macros.contains(cursor.advance().value)) {
                    size_t depth = 1;
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE && depth > 0) {
                        if (auto token = cursor.advance(); token.value == "\\ifx" || token.value == "\\ifnum" || token.value == "\\ifdim" || token.value == "\\ifdefined") {
                            depth++;
                        } else if (token.value == "\\fi") {
                            if (depth == 1) break;
                            depth--;
                        } else if (token.value == "\\else") {
                            if (depth == 1) break;
                        }
                    }
                }
                return nullptr;
            }

            if (cursor.lookahead().value == "\\else") {
                cursor.advance();
                size_t depth = 1;
                while (cursor.lookahead().type != lexer::Type::END_OF_FILE && depth > 0) {
                    auto token = cursor.advance();
                    if (token.value == "\\ifx" || token.value == "\\ifnum" || token.value == "\\ifdim" || token.value == "\\ifdefined") depth++;
                    if (token.value == "\\fi") depth--;
                }
                return nullptr;
            }

            if (cursor.lookahead().value == "\\fi") {
                cursor.advance();
                return nullptr;
            }

            if (macros.contains(cursor.lookahead().value)) {
                const auto& body = macros[cursor.advance().value];
                std::vector<lexer::Token> tokens;
                tokens.reserve(body.size());
                for (const auto& token : body) {
                    tokens.push_back(token);
                }
                cursor.inject(std::move(tokens));
                return this->node();
            }

            if (cursor.lookahead().value == "\\$" || cursor.lookahead().value == "\\%" || cursor.lookahead().value == "\\{" ||
                cursor.lookahead().value == "\\}" || cursor.lookahead().value == "\\&" || cursor.lookahead().value == "\\_" ||
                cursor.lookahead().value == "\\^" || cursor.lookahead().value == "\\#") {
                return arena.construct<ast::Node>(ast::Type::TEXT, cursor.advance().value.substr(1));
            }

            if (cursor.lookahead().value == "\\") {
                if (cursor.lookahead(1).type == lexer::Type::LEFT_BRACKET) {
                    auto* node = arena.construct<ast::Node>(ast::Type::MATH_DISPLAY, cursor.advance().value);
                    cursor.advance();
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
                        if (cursor.lookahead().type == lexer::Type::MACRO && cursor.lookahead().value == "\\" && cursor.lookahead(1).type == lexer::Type::RIGHT_BRACKET) {
                            cursor.advance();
                            cursor.advance();
                            break;
                        }
                        if (auto* child = this->node()) {
                            node->nodes.push_back(child);
                        }
                    }
                    return node;
                }

                if (cursor.lookahead(1).type == lexer::Type::LEFT_PARENTHESIS) {
                    auto* node = arena.construct<ast::Node>(ast::Type::MATH_INLINE, cursor.advance().value);
                    cursor.advance();
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
                        if (cursor.lookahead().type == lexer::Type::MACRO && cursor.lookahead().value == "\\" && cursor.lookahead(1).type == lexer::Type::RIGHT_PARENTHESIS) {
                            cursor.advance();
                            cursor.advance();
                            break;
                        }
                        if (auto* child = this->node()) {
                            node->nodes.push_back(child);
                        }
                    }
                    return node;
                }
            }

            if (cursor.lookahead().value == "\\begin") {
                cursor.advance();
                if (cursor.lookahead().type == lexer::Type::LEFT_BRACE) {
                    cursor.advance();
                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                        cursor.advance();
                    }

                    if (cursor.advance().value == "verbatim") {
                        auto* node = arena.construct<ast::Node>(ast::Type::VERBATIM, cursor.advance().value);
                        std::string_view stream;
                        while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
                            if (cursor.lookahead().type == lexer::Type::MACRO && cursor.lookahead().value == "\\end") {
                                if (cursor.lookahead(1).type == lexer::Type::LEFT_BRACE && cursor.lookahead(2).value == "verbatim") {
                                    cursor.advance();
                                    cursor.advance();
                                    cursor.advance();
                                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                                        cursor.advance();
                                    }
                                    break;
                                }
                            }
                            auto active = cursor.advance();
                            if (stream.empty()) {
                                stream = active.value;
                            } else {
                                stream = std::string_view(stream.data(), active.value.data() - stream.data() + active.value.length());
                            }
                        }
                        node->nodes.push_back(arena.construct<ast::Node>(ast::Type::TEXT, stream));
                        return node;
                    }

                    auto name = cursor.advance().value;
                    auto* node = arena.construct<ast::Node>(ast::Type::ENVIRONMENT, name);
                    while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
                        if (cursor.lookahead().type == lexer::Type::MACRO && cursor.lookahead().value == "\\end") {
                            if (cursor.lookahead(1).type == lexer::Type::LEFT_BRACE && cursor.lookahead(2).value == name) {
                                cursor.advance();
                                cursor.advance();
                                cursor.advance();
                                if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                                    cursor.advance();
                                }
                                break;
                            }
                        }
                        if (auto* child = this->node()) {
                            node->nodes.push_back(child);
                        }
                    }
                    return node;
                }
            }

            auto* node = arena.construct<ast::Node>(ast::Type::MACRO, cursor.advance().value);

            while (true) {
                if (cursor.lookahead().type == lexer::Type::LEFT_BRACKET) {
                    auto* child = arena.construct<ast::Node>(ast::Type::BLOCK, cursor.advance().value);
                    child->nodes = query(lexer::Type::RIGHT_BRACKET);
                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACKET) {
                        cursor.advance();
                    }
                    node->nodes.push_back(child);
                } else if (cursor.lookahead().type == lexer::Type::LEFT_BRACE) {
                    auto* child = arena.construct<ast::Node>(ast::Type::SCOPE, cursor.advance().value);
                    child->nodes = query(lexer::Type::RIGHT_BRACE);
                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                        cursor.advance();
                    }
                    node->nodes.push_back(child);
                } else {
                    break;
                }
            }
            return node;
        }

        if (cursor.lookahead().type == lexer::Type::CARET) {
            auto* node = arena.construct<ast::Node>(ast::Type::SUPERSCRIPT, cursor.advance().value);
            if (auto* child = this->node()) {
                node->nodes.push_back(child);
            }
            return node;
        }

        if (cursor.lookahead().type == lexer::Type::UNDERSCORE) {
            auto* node = arena.construct<ast::Node>(ast::Type::SUBSCRIPT, cursor.advance().value);
            if (auto* child = this->node()) {
                node->nodes.push_back(child);
            }
            return node;
        }

        if (cursor.lookahead().type == lexer::Type::AMPERSAND) {
            return arena.construct<ast::Node>(ast::Type::ALIGNMENT_MARKER, cursor.advance().value);
        }

        if (cursor.lookahead().type == lexer::Type::ROW_BREAK) {
            return arena.construct<ast::Node>(ast::Type::ROW_BREAK_MARKER, cursor.advance().value);
        }

        return arena.construct<ast::Node>(ast::Type::TEXT, cursor.advance().value);
    }

}