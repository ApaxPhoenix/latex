#include "syntax/cursor.hpp"
#include "logger.hpp"

namespace syntax {

    Cursor::Cursor(std::vector<Token> tokens) {
        if (!tokens.empty()) {
            this->tokens = std::move(tokens);
            this->head = 0;
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "Cursor stream initialized (size={})", this->tokens.size());
        }
    }

    Token Cursor::lookahead(const std::size_t offset) const noexcept {
        if (this->head + offset < this->tokens.size()) {
            return this->tokens[this->head + offset];
        }
        return {};
    }

    Token Cursor::advance() noexcept {
        if (this->head < this->tokens.size()) {
            Token token = this->tokens[this->head++];
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback, "Cursor -> [head={}, text={}]", this->head, token.symbol);
            return token;
        }
        return {};
    }

    void Cursor::inject(const std::span<const Token> tokens) {
        if (tokens.empty()) return;
        this->tokens.insert(this->tokens.begin() + static_cast<std::ptrdiff_t>(this->head), tokens.begin(), tokens.end());
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "Cursor injected {} tokens at head {}", tokens.size(), this->head);
    }

    bool Cursor::empty() const noexcept {
        return this->head >= this->tokens.size();
    }

}