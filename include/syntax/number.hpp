#pragma once

#include "syntax/tokens.hpp"
#include "syntax/cursor.hpp"
#include "semantics/registers.hpp"

#include <string_view>
#include <optional>

namespace syntax {

    class Number {
    public:
        static constexpr std::int32_t scale = 65536;

        static std::optional<std::int32_t> integer(Cursor& cursor, const semantics::Registers& registers, const Symbol count) {
            int sign = 1;
            while (!cursor.empty()) {
                if (const Token token = cursor.lookahead(0); token.value == "+") { cursor.advance(); }
                else if (token.value == "-") { sign = -sign; cursor.advance(); }
                else break;
            }

            if (cursor.empty()) return std::nullopt;

            if (const Token token = cursor.lookahead(0); token.symbol == count) {
                cursor.advance();
                const auto index = integer(cursor, registers, count);
                if (!index) return std::nullopt;
                return registers.fetch(semantics::Registers::Type::Count, static_cast<std::size_t>(*index)) * sign;
            }

            if (const Token token = cursor.lookahead(0); token.type == CatCodes::Type::Escape) {
                const auto val = registers.get(token.symbol);
                cursor.advance();
                return val * sign;
            }

            const Token token = cursor.advance();
            std::int32_t value = 0;

            if (token.value.starts_with("'")) {
                for (const std::string_view text = token.value.substr(1); const char symbol : text) {
                    if (symbol >= '0' && symbol <= '7') value = value * 8 + (symbol - '0');
                    else break;
                }
            } else if (token.value.starts_with("\"")) {
                for (const std::string_view text = token.value.substr(1); const char symbol : text) {
                    if (symbol >= '0' && symbol <= '9') value = value * 16 + (symbol - '0');
                    else if (symbol >= 'A' && symbol <= 'F') value = value * 16 + (symbol - 'A' + 10);
                    else if (symbol >= 'a' && symbol <= 'f') value = value * 16 + (symbol - 'a' + 10);
                    else break;
                }
            } else {
                for (const char symbol : token.value) {
                    if (symbol >= '0' && symbol <= '9') value = value * 10 + (symbol - '0');
                    else break;
                }
            }

            return value * sign;
        }

        static std::optional<std::int32_t> dimension(Cursor& cursor, const semantics::Registers& registers, const Symbol count, const Symbol dimen) {
            if (!cursor.empty() && cursor.lookahead(0).symbol == dimen) {
                cursor.advance();
                const auto index = integer(cursor, registers, count);
                if (!index) return std::nullopt;
                return registers.fetch(semantics::Registers::Type::Dimension, static_cast<std::size_t>(*index));
            }

            const auto raw = integer(cursor, registers, count);
            if (!raw) return std::nullopt;

            double points = *raw;
            if (!cursor.empty() && cursor.lookahead(0).value == ".") {
                cursor.advance();
                if (!cursor.empty()) {
                    const Token token = cursor.advance();
                    double fraction = 0.0;
                    double divisor = 10.0;
                    for (const char symbol : token.value) {
                        if (symbol >= '0' && symbol <= '9') {
                            fraction += (symbol - '0') / divisor;
                            divisor *= 10.0;
                        } else break;
                    }
                    points += fraction;
                }
            }

            if (cursor.empty()) return static_cast<std::int32_t>(points * scale);

            const Token token = cursor.advance();
            const std::string_view unit = token.value;

            double factor = scale;
            if (unit == "pt") factor = scale;
            else if (unit == "pc") factor = scale * 12.0;
            else if (unit == "in") factor = scale * 72.27;
            else if (unit == "bp") factor = scale * (72.27 / 72.0);
            else if (unit == "cm") factor = scale * (72.27 / 2.54);
            else if (unit == "mm") factor = scale * (72.27 / 25.4);
            else if (unit == "sp") factor = 1.0;

            return static_cast<std::int32_t>(points * factor);
        }
    };

}