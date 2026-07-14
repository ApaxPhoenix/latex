#pragma once

#include <vector>
#include <string_view>

namespace core::lexer {

    enum class Type {
        // Delimiters and operators
        LEFT_BRACE,
        RIGHT_BRACE,
        LEFT_BRACKET,
        RIGHT_BRACKET,
        LEFT_PARENTHESIS,
        RIGHT_PARENTHESIS,
        AMPERSAND,
        UNDERSCORE,
        CARET,
        TILDE,
        ROW_BREAK,
        DOLLAR,

        // Command prefixes
        ESCAPE,
        MACRO,
        PARAMETER,
        REFERENCE,

        // Basic content
        LETTER,
        OTHER,
        PROSE,
        TEXT,

        // Document syntax
        COMMENT,
        IDENTIFIER,
        NEWLINE,
        WHITESPACE,
        END_OF_FILE
    };

    struct Token {
        Type type;
        std::string_view value;
        size_t line;
        size_t column;
    };

    class Lexer {
    public:
        explicit Lexer(std::string_view source);
        std::vector<Token> tokenize();

    private:
        std::string_view source;
        size_t start = 0;
        size_t end = 0;
        size_t line = 1;
        size_t column = 1;
        Type categories[256];
        Token advance();
    };

    class Cursor {
    public:
        Cursor() = default;
        explicit Cursor(std::vector<Token> tokens);
        Token advance();
        [[nodiscard]] Token lookahead(size_t distance = 0) const;
        void inject(std::vector<Token>&& items);

    private:
        std::vector<Token> tokens;
        size_t index = 0;
    };

}