#pragma once

#include <vector>
#include <string_view>

namespace core::lexer {

    enum class Type {
        LEFT_BRACE,        // {  -> Mandatory argument / block scope opener
        RIGHT_BRACE,       // }  -> Mandatory argument / block scope closer
        LEFT_BRACKET,      // [  -> Optional key-value argument or array dimension opener
        RIGHT_BRACKET,     // ]  -> Optional key-value argument or array dimension closer
        LEFT_PARENTHESIS,  // (  -> Macro parameter or explicit grouping opener
        RIGHT_PARENTHESIS, // )  -> Macro parameter or explicit grouping closer
        ROW_BREAK,         // \\ -> Explicit line/row break control sequence

        DOLLAR,            // $  -> Standard inline math mode toggle (Modern display math uses \[ ... \])
        AMPERSAND,         // &  -> Alignment tab character for modern tables/matrices (e.g., align, tabular)
        UNDERSCORE,        // _  -> Subscript assignment operator (valid in math scopes)
        CARET,             // ^  -> Superscript assignment operator (valid in math scopes)
        TILDE,             // ~  -> Non-breaking tie space
        COMMENT,           // %  -> Line comment marker (discards everything to end of line)

        COMMAND,           // \name -> Control sequence, macro, or backslash-escaped character (e.g., \&, \%)
        IDENTIFIER,        // Generic symbols, individual alphanumeric glyphs, or math operators (+, -, =, etc.)
        PROSE,              // Contiguous block of raw literal text characters outside of math mode

        NEWLINE,           // Explicit vertical line break character (\n or \r\n)
        WHITESPACE,        // Contiguous horizontal blanks or tabs (\t or spaces)
        END_OF_FILE,       // Sentinel marker indicating the end of the source stream
    };

    struct Token {
        Type type;
        std::string_view value; // Points directly into original source buffer
        size_t line;            // Vertical coordinate for diagnostics
        size_t column;          // Horizontal coordinate for diagnostics
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
        size_t column = 1; // Track character offset relative to current line break

        Token advance();
    };

    class Cursor {
    public:
        explicit Cursor(std::vector<Token> tokens);

        Token advance();
        [[nodiscard]] Token lookahead(size_t distance = 0) const;

    private:
        std::vector<Token> tokens;
        size_t index = 0;
    };

}