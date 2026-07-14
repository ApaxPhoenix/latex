#include "core/lexer.hpp"

namespace core::lexer {

    Cursor::Cursor(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

    Token Cursor::lookahead(const size_t distance) const {
        if (index + distance < tokens.size()) {
            return tokens[index + distance];
        }
        if (!tokens.empty()) {
            return tokens.back();
        }
        return Token{Type::END_OF_FILE, "", 1, 1};
    }

    Token Cursor::advance() {
        if (index < tokens.size() && tokens[index].type != Type::END_OF_FILE) {
            return tokens[index++];
        }
        return lookahead(0);
    }

    void Cursor::inject(std::vector<Token>&& items) {
        if (items.empty()) return;
        tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(index), items.begin(), items.end());
    }
}