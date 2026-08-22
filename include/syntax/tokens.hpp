#pragma once

#include "syntax/catcodes.hpp"
#include "syntax/lexicon.hpp"
#include "memory/location.hpp"

#include <string_view>
#include <type_traits>

namespace syntax {

    struct Token {
        Symbol symbol = kInvalidSymbol;
        CatCodes::Category category = CatCodes::Category::Invalid;
        memory::Location location{};
        std::string_view values{};

        [[nodiscard]] constexpr bool empty() const noexcept {
            return this->values.empty() && this->symbol == kInvalidSymbol;
        }

        [[nodiscard]] constexpr bool operator==(const Token&) const noexcept = delete;
    };

    static_assert(std::is_trivially_copyable_v<Token>, "Token must remain trivially copyable");

}