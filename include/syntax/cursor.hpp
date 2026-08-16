#pragma once

#include "syntax/tokens.hpp"

#include <span>
#include <vector>

namespace syntax {

    class Cursor {
    public:
        Cursor() = default;
        explicit Cursor(std::vector<Token> tokens);

        [[nodiscard]] Token lookahead(std::size_t offset = 0) const noexcept;
        Token advance() noexcept;
        void inject(std::span<const Token> tokens);
        [[nodiscard]] bool empty() const noexcept;

    private:
        std::vector<Token> tokens{};
        std::size_t head = 0;
    };

}