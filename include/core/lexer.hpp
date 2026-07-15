#pragma once

#include <vector>
#include <string_view>

namespace core::lexer {

    enum class Type {
        LEFT_BRACE,         // '{'
        RIGHT_BRACE,        // '}'
        LEFT_BRACKET,       // '['
        RIGHT_BRACKET,      // ']'
        LEFT_PARENTHESIS,   // '('
        RIGHT_PARENTHESIS,  // ')'
        AMPERSAND,          // '&'
        UNDERSCORE,         // '_'
        CARET,              // '^'
        TILDE,              // '~'
        ROW_BREAK,          // '\\'
        DOLLAR,             // '$'
        ESCAPE,             // '\'
        MACRO,              // '\command'
        PARAMETER,          // '#1'
        REFERENCE,          // '@ref'
        TEXT,               // letters
        NUMBER,             // digits
        COMMENT,            // '%'
        NEWLINE,            // '\n'
        WHITESPACE,         // ' '
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
        size_t start;
        size_t end;
        size_t line;
        size_t column;
        Type categories[256];
        Token advance();
    };

    struct Stream {
        std::vector<Token> list;
        size_t head = 0;
    };

    class Cursor {
    public:
        Cursor() = default;
        explicit Cursor(std::vector<Token> tokens);
        Token advance();
        [[nodiscard]] Token lookahead(size_t distance = 0) const;
        void inject(std::vector<Token>&& items);

    private:
        std::vector<Stream> stack;
    };

}