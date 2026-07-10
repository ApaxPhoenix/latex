#include "lexer.hpp"
#include <utility>

namespace core::lexer {

    Lexer::Lexer(const std::string_view source) : source(source) {}

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        while (true) {
            Token token = this->advance();
            tokens.push_back(token);
            if (token.type == Type::END_OF_FILE) {
                break;
            }
        }
        return tokens;
    }

    Token Lexer::advance() {
        this->start = this->end;

        while (this->end < this->source.length()) {
            if (this->source[this->end] == '\n' || this->source[this->end] == '\r') {
                if (this->source[this->end] == '\r' && (this->end + 1) < this->source.length() && this->source[this->end + 1] == '\n') {
                    this->end++;
                }
                this->end++;
                this->line++;
                return Token{Type::NEWLINE, this->source.substr(this->start, this->end - this->start), this->line - 1, std::exchange(this->column, 1)};
            }

            if (this->source[this->end] == ' ' || this->source[this->end] == '\t') {
                while (this->end < this->source.length() && (this->source[this->end] == ' ' || this->source[this->end] == '\t')) {
                    this->end++;
                    this->column++;
                }
                return Token{Type::WHITESPACE, this->source.substr(this->start, this->end - this->start), this->line, this->column - (this->end - this->start)};
            }

            if (std::isspace(static_cast<unsigned char>(this->source[this->end]))) {
                this->end++;
                this->column++;
                this->start = this->end;
            } else {
                break;
            }
        }

        if (this->end >= this->source.length()) {
            return Token{Type::END_OF_FILE, "", this->line, this->column};
        }

        if (this->source[this->end] == '{') { this->end++; this->column++; return Token{Type::LEFT_BRACE, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '}') { this->end++; this->column++; return Token{Type::RIGHT_BRACE, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '[') { this->end++; this->column++; return Token{Type::LEFT_BRACKET, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == ']') { this->end++; this->column++; return Token{Type::RIGHT_BRACKET, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '(') { this->end++; this->column++; return Token{Type::LEFT_PARENTHESIS, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == ')') { this->end++; this->column++; return Token{Type::RIGHT_PARENTHESIS, this->source.substr(this->start, 1), this->line, this->column - 1}; }

        if (this->source[this->end] == '$') {
            this->end++; this->column++;
            return Token{Type::DOLLAR, this->source.substr(this->start, 1), this->line, this->column - 1};
        }
        if (this->source[this->end] == '&') { this->end++; this->column++; return Token{Type::AMPERSAND, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '_') { this->end++; this->column++; return Token{Type::UNDERSCORE, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '^') { this->end++; this->column++; return Token{Type::CARET, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '~') { this->end++; this->column++; return Token{Type::TILDE, this->source.substr(this->start, 1), this->line, this->column - 1}; }
        if (this->source[this->end] == '%') { this->end++; this->column++; return Token{Type::COMMENT, this->source.substr(this->start, 1), this->line, this->column - 1}; }

        if (this->source[this->end] == '\\') {
            this->end++;
            this->column++;
            if (this->end < this->source.length()) {
                if (this->source[this->end] == '\\') {
                    this->end++;
                    this->column++;
                    return Token{Type::ROW_BREAK, this->source.substr(this->start, 2), this->line, this->column - 2};
                }

                if (std::isalpha(static_cast<unsigned char>(this->source[this->end]))) {
                    while (this->end < this->source.length() && std::isalpha(static_cast<unsigned char>(this->source[this->end]))) {
                        this->end++;
                        this->column++;
                    }
                    return Token{Type::COMMAND, this->source.substr(this->start, this->end - this->start), this->line, this->column - (this->end - this->start)};
                }

                this->end++;
                this->column++;
                return Token{Type::COMMAND, this->source.substr(this->start, 2), this->line, this->column - 2};
            }
            return Token{Type::COMMAND, this->source.substr(this->start, 1), this->line, this->column - 1};
        }

        if (std::string_view("+-*/=,0123456789").contains(this->source[this->end])) {
            this->end++;
            this->column++;
            return Token{Type::IDENTIFIER, this->source.substr(this->start, 1), this->line, this->column - 1};
        }

        while (this->end < this->source.length() &&
               !std::isspace(static_cast<unsigned char>(this->source[this->end])) &&
               !std::string_view("{}[]()$&\\_^~%+-*/=,").contains(this->source[this->end]) &&
               !std::isdigit(static_cast<unsigned char>(this->source[this->end]))) {
            this->end++;
            this->column++;
        }

        const std::string_view slice = this->source.substr(this->start, this->end - this->start);
        return Token{Type::PROSE, slice, this->line, this->column - slice.length()};
    }
}