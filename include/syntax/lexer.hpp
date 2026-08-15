#pragma once

#include "syntax/catcodes.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/tokens.hpp"
#include "memory/location.hpp"

#include <cstddef>
#include <string_view>

namespace syntax {

    class Lexer {
    public:
        enum class Type {
            Newline,
            Skip,
            Middle
        };

        Lexer(std::string_view source, CatCodes& table, Lexicon& lexicon);

        Token advance();
        [[nodiscard]] bool empty() const noexcept;
        void reset() noexcept;

    private:
        [[nodiscard]] static constexpr std::size_t length(const char lead) noexcept {
            const auto byte = static_cast<unsigned char>(lead);
            if ((byte & 0x80) == 0) return 1;
            if ((byte & 0xE0) == 0xC0) return 2;
            if ((byte & 0xF0) == 0xE0) return 3;
            if ((byte & 0xF8) == 0xF0) return 4;
            return 1;
        }

        std::string_view source{};
        CatCodes& table;
        Lexicon& lexicon;
        std::size_t offset = 0;
        memory::Location location{1, 1};
        Type type = Type::Newline;
    };

}