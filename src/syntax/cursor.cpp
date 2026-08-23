#include "syntax/cursor.hpp"
#include "logger.hpp"

#include <algorithm>
#include <utility>

namespace syntax {

    Cursor::Cursor(std::vector<Token> tokens) {
        if (!tokens.empty()) {
            this->tokens = std::move(tokens);
            std::ranges::reverse(this->tokens);
            this->head = 0;
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "Cursor stream initialized (size={})", this->tokens.size());
        }
    }

    Token Cursor::lookahead(const std::size_t offset) const noexcept {
        if (offset < this->tokens.size()) {
            return this->tokens[this->tokens.size() - 1 - offset];
        }
        return {};
    }

    Token Cursor::advance() noexcept {
        if (!this->tokens.empty()) {
            Token token = this->tokens.back();
            this->tokens.pop_back();
            this->head++;
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback, "Cursor -> [head={}, text={}]", this->head, token.symbol);
            if (this->tokens.empty()) {
                this->head = 0;
            }
            return token;
        }
        return {};
    }

    void Cursor::inject(const std::span<const Token> tokens) {
        if (tokens.empty()) return;
        this->tokens.insert(this->tokens.end(), tokens.rbegin(), tokens.rend());
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "Cursor injected {} tokens at head {}", tokens.size(), this->head);
    }

    bool Cursor::empty() const noexcept {
        return this->tokens.empty();
    }

}