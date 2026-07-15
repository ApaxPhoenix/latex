#include "core/lexer.hpp"

#include <utility>
#include <iostream>

namespace core::lexer {

    Lexer::Lexer(const std::string_view source)
        : source(source), start(0), end(0), line(1), column(1), categories{} {
        for (auto& element : categories) {
            element = Type::TEXT;
        }

        for (int i = '0'; i <= '9'; ++i) {
            categories[i] = Type::NUMBER;
        }

        categories['\\'] = Type::ESCAPE;
        categories['#'] = Type::PARAMETER;
        categories['{'] = Type::LEFT_BRACE;
        categories['}'] = Type::RIGHT_BRACE;
        categories['['] = Type::LEFT_BRACKET;
        categories[']'] = Type::RIGHT_BRACKET;
        categories['('] = Type::LEFT_PARENTHESIS;
        categories[')'] = Type::RIGHT_PARENTHESIS;
        categories['&'] = Type::AMPERSAND;
        categories['^'] = Type::CARET;
        categories['_'] = Type::UNDERSCORE;
        categories['~'] = Type::TILDE;
        categories['$'] = Type::DOLLAR;
    }

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

        if (this->end >= this->source.length()) {
            return Token{Type::END_OF_FILE, "", this->line, this->column};
        }

        const Type state = categories[static_cast<unsigned char>(this->source[this->end])];

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

        if (this->source[this->end] == '%') {
            while (this->end < this->source.length() && this->source[this->end] != '\n' && this->source[this->end] != '\r') {
                this->end++;
                this->column++;
            }
            return Token{Type::COMMENT, this->source.substr(this->start, this->end - this->start), this->line, this->column - (this->end - this->start)};
        }

        if (state == Type::DOLLAR) {
            if (this->end + 1 < this->source.length() && this->source[this->end + 1] == '$') {
                std::cerr << "Warning: on line " << this->line << ", column " << column << ": '$$' is deprecated. Use '\\[ ... \\]' instead.\n";
                this->end += 2;
                this->column += 2;
                return Token{Type::DOLLAR, this->source.substr(this->start, 2), this->line, column};
            }

            std::cerr << "Warning: on line " << this->line << ", column " << column << ": '$' is deprecated. Use '\\( ... \\)' instead.\n";
            this->end++;
            this->column++;
            return Token{Type::DOLLAR, this->source.substr(this->start, 1), this->line, column};
        }

        if (state == Type::PARAMETER) {
            if (this->end + 1 < this->source.length() && this->source[this->end + 1] >= '1' && this->source[this->end + 1] <= '9') {
                this->end += 2;
                this->column += 2;
                return Token{Type::REFERENCE, this->source.substr(this->start, 2), this->line, this->column - 2};
            }
        }

        if (state == Type::NUMBER) {
            while (this->end < this->source.length() &&
                   categories[static_cast<unsigned char>(this->source[this->end])] == Type::NUMBER) {
                this->end++;
                this->column++;
            }

            if (this->end + 1 < this->source.length() &&
                this->source[this->end] == '.' &&
                categories[static_cast<unsigned char>(this->source[this->end + 1])] == Type::NUMBER) {

                this->end++;
                this->column++;

                while (this->end < this->source.length() &&
                       categories[static_cast<unsigned char>(this->source[this->end])] == Type::NUMBER) {
                    this->end++;
                    this->column++;
                }
            }

            return Token{Type::NUMBER, this->source.substr(this->start, this->end - this->start), this->line, this->column - (this->end - this->start)};
        }

        if (state == Type::ESCAPE) {
            if (this->end + 1 < this->source.length() && this->source[this->end + 1] == '\\') {
                this->end += 2;
                this->column += 2;
                return Token{Type::ROW_BREAK, this->source.substr(this->start, 2), this->line, this->column - 2};
            }
            this->end++;
            this->column++;
            if (this->end < this->source.length()) {
                if ((this->source[this->end] >= 'a' && this->source[this->end] <= 'z') || (this->source[this->end] >= 'A' && this->source[this->end] <= 'Z')) {
                    while (this->end < this->source.length()) {
                        if ((this->source[this->end] >= 'a' && this->source[this->end] <= 'z') || (this->source[this->end] >= 'A' && this->source[this->end] <= 'Z')) {
                            this->end++;
                            this->column++;
                        } else {
                            break;
                        }
                    }
                    return Token{Type::MACRO, this->source.substr(this->start, this->end - this->start), this->line, this->column - (this->end - this->start)};
                }
                this->end++;
                this->column++;
                return Token{Type::MACRO, this->source.substr(this->start, 2), this->line, this->column - 2};
            }
            return Token{Type::MACRO, this->source.substr(this->start, 1), this->line, this->column - 1};
        }

        if (state != Type::TEXT) {
            this->end++;
            this->column++;
            return Token{state, this->source.substr(this->start, 1), this->line, this->column - 1};
        }

        this->end++;
        this->column++;
        return Token{Type::TEXT, this->source.substr(this->start, 1), this->line, this->column - 1};
    }

}