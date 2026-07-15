#include "core/lexer.hpp"

#include <ranges>

namespace core::lexer {

    Cursor::Cursor(std::vector<Token> tokens) {
        if (!tokens.empty()) {
            stack.push_back(Stream{std::move(tokens), 0});
        }
    }

    Token Cursor::lookahead(size_t distance) const {
        for (const auto &[list, head] : std::views::reverse(stack)) {
            const size_t remaining = list.size() - head;
            if (distance < remaining) {
                return list[head + distance];
            }
            distance -= remaining;
        }
        if (!stack.empty() && !stack.front().list.empty()) {
            return stack.front().list.back();
        }
        return Token{Type::END_OF_FILE, "", 1, 1};
    }

    Token Cursor::advance() {
        while (!stack.empty()) {
            if (auto&[list, head] = stack.back(); head < list.size()) {
                const Token token = list[head];
                if (token.type == Type::END_OF_FILE && stack.size() > 1) {
                    stack.pop_back();
                    continue;
                }
                head++;
                return token;
            }
            stack.pop_back();
        }
        return Token{Type::END_OF_FILE, "", 1, 1};
    }

    void Cursor::inject(std::vector<Token>&& items) {
        if (items.empty()) {
            return;
        }
        stack.push_back(Stream{std::move(items), 0});
    }

}