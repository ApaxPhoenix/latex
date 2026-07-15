#include "core/parser.hpp"

#include <charconv>
#include <string_view>

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

    std::vector<ast::Node*> Parser::query(const lexer::Type target) {
        std::vector<ast::Node*> list;

        while (cursor.lookahead().type != lexer::Type::END_OF_FILE && cursor.lookahead().type != target) {
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
            auto* node = arena.construct<ast::Node>(ast::Type::GROUP, cursor.advance().value);
            node->nodes = query(lexer::Type::RIGHT_BRACE);
            if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                cursor.advance();
            }
            return node;
        }

        if (cursor.lookahead().type == lexer::Type::MACRO) {
            if (cursor.lookahead().value == "\\ifx") {
                cursor.advance();
                auto left = cursor.advance();
                if (auto right = cursor.advance(); left.type != right.type || left.value != right.value) {
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
                auto left = cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto relation = cursor.advance().value;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto right = cursor.advance();

                int first = 0, second = 0;
                std::from_chars(left.value.data(), left.value.data() + left.value.size(), first);
                std::from_chars(right.value.data(), right.value.data() + right.value.size(), second);

                if ((relation == "<" && first >= second) || (relation == ">" && first <= second) || (relation == "=" && first != second)) {
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
                auto left = cursor.advance();
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto relation = cursor.advance().value;
                while (cursor.lookahead().type == lexer::Type::WHITESPACE) cursor.advance();
                auto right = cursor.advance();

                double measure = 0.0;
                std::string_view unit;
                {
                    std::string text(left.value);
                    char* pointer = nullptr;
                    measure = std::strtod(text.c_str(), &pointer);
                    if (pointer != text.c_str()) {
                        unit = left.value.substr(pointer - text.c_str());
                        if (const auto position = unit.find_first_not_of(" \t"); position != std::string_view::npos) {
                            unit.remove_prefix(position);
                        }
                    }
                }

                double amount = 0.0;
                std::string_view suffix;
                {
                    std::string text(right.value);
                    char* pointer = nullptr;
                    amount = std::strtod(text.c_str(), &pointer);
                    if (pointer != text.c_str()) {
                        suffix = right.value.substr(pointer - text.c_str());
                        if (const auto position = suffix.find_first_not_of(" \t"); position != std::string_view::npos) {
                            suffix.remove_prefix(position);
                        }
                    }
                }

                double factor = 1.0;
                if (unit == "pc") factor = 12.0;
                else if (unit == "in") factor = 72.27;
                else if (unit == "bp") factor = 72.27 / 72.0;
                else if (unit == "cm") factor = 72.27 / 2.54;
                else if (unit == "mm") factor = 72.27 / 25.4;
                else if (unit == "dd") factor = 1238.0 / 1157.0;
                else if (unit == "cc") factor = 12.0 * 1238.0 / 1157.0;
                else if (unit == "sp") factor = 1.0 / 65536.0;
                else if (unit == "em") factor = 10.0;
                else if (unit == "ex") factor = 4.3;
                measure *= factor;

                factor = 1.0;
                if (suffix == "pc") factor = 12.0;
                else if (suffix == "in") factor = 72.27;
                else if (suffix == "bp") factor = 72.27 / 72.0;
                else if (suffix == "cm") factor = 72.27 / 2.54;
                else if (suffix == "mm") factor = 72.27 / 25.4;
                else if (suffix == "dd") factor = 1238.0 / 1157.0;
                else if (suffix == "cc") factor = 12.0 * 1238.0 / 1157.0;
                else if (suffix == "sp") factor = 1.0 / 65536.0;
                else if (suffix == "em") factor = 10.0;
                else if (suffix == "ex") factor = 4.3;
                amount *= factor;

                if ((relation == "<" && measure >= amount) ||
                    (relation == ">" && measure <= amount) ||
                    (relation == "=" && measure != amount)) {
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
                    auto* node = arena.construct<ast::Node>(ast::Type::DISPLAY, cursor.advance().value);
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
                    auto* node = arena.construct<ast::Node>(ast::Type::INLINE, cursor.advance().value);
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

                    auto token = cursor.advance();

                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACE) {
                        cursor.advance();
                    }

                    if (token.value == "document") {
                        auto* node = arena.construct<ast::Node>(ast::Type::DOCUMENT, token.value);
                        while (cursor.lookahead().type != lexer::Type::END_OF_FILE) {
                            if (cursor.lookahead().type == lexer::Type::MACRO && cursor.lookahead().value == "\\end") {
                                if (cursor.lookahead(1).type == lexer::Type::LEFT_BRACE && cursor.lookahead(2).value == "document") {
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

                    if (token.value == "verbatim") {
                        auto* node = arena.construct<ast::Node>(ast::Type::VERBATIM, token.value);
                        std::string_view span;
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
                            auto next = cursor.advance();
                            if (span.empty()) {
                                span = next.value;
                            } else {
                                span = std::string_view(span.data(), next.value.data() - span.data() + next.value.length());
                            }
                        }
                        node->nodes.push_back(arena.construct<ast::Node>(ast::Type::TEXT, span));
                        return node;
                    }
                }
            }

            auto* node = arena.construct<ast::Node>(ast::Type::MACRO, cursor.advance().value);

            while (true) {
                if (cursor.lookahead().type == lexer::Type::LEFT_BRACKET) {
                    auto* child = arena.construct<ast::Node>(ast::Type::GROUP, cursor.advance().value);
                    child->nodes = query(lexer::Type::RIGHT_BRACKET);
                    if (cursor.lookahead().type == lexer::Type::RIGHT_BRACKET) {
                        cursor.advance();
                    }
                    node->nodes.push_back(child);
                } else if (cursor.lookahead().type == lexer::Type::LEFT_BRACE) {
                    auto* child = arena.construct<ast::Node>(ast::Type::GROUP, cursor.advance().value);
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
            return arena.construct<ast::Node>(ast::Type::ALIGN, cursor.advance().value);
        }

        if (cursor.lookahead().type == lexer::Type::ROW_BREAK) {
            return arena.construct<ast::Node>(ast::Type::BREAK, cursor.advance().value);
        }

        return arena.construct<ast::Node>(ast::Type::TEXT, cursor.advance().value);
    }

}