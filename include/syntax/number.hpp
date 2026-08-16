#pragma once

#include "syntax/tokens.hpp"
#include "syntax/cursor.hpp"
#include "semantics/registers.hpp"

#include <cstdint>
#include <string_view>
#include <optional>

namespace syntax {

    class Number {
    public:
        static constexpr std::int32_t scale = 65536;

        [[nodiscard]] static std::optional<std::int32_t> integer(Cursor& cursor, const semantics::Registers& registers, const Symbol count) {
            int sign = 1;

            while (!cursor.empty()) {
                if (const Token token = cursor.lookahead(0); token.values == "+") {
                    cursor.advance();
                } else if (token.values == "-") {
                    sign = -sign;
                    cursor.advance();
                } else {
                    break;
                }
            }

            if (cursor.empty()) return std::nullopt;

            const Token lead = cursor.lookahead(0);

            if (lead.symbol == count) {
                cursor.advance();
                const auto index = integer(cursor, registers, count);
                if (!index) return std::nullopt;
                return registers.fetch(semantics::Registers::Type::Count, static_cast<std::size_t>(*index)) * sign;
            }

            if (lead.category == CatCodes::Category::Escape) {
                const auto value = registers.get(lead.symbol);
                cursor.advance();
                return value * sign;
            }

            const Token token = cursor.advance();
            std::int32_t value = 0;

            if (token.values.starts_with("'")) {
                for (const char symbol : token.values.substr(1)) {
                    if (symbol >= '0' && symbol <= '7') value = (value << 3) + (symbol - '0');
                    else break;
                }
            } else if (token.values.starts_with("\"")) {
                for (const char symbol : token.values.substr(1)) {
                    if (symbol >= '0' && symbol <= '9') value = (value << 4) + (symbol - '0');
                    else if (symbol >= 'A' && symbol <= 'F') value = (value << 4) + (symbol - 'A' + 10);
                    else if (symbol >= 'a' && symbol <= 'f') value = (value << 4) + (symbol - 'a' + 10);
                    else break;
                }
            } else {
                for (const char symbol : token.values) {
                    if (symbol >= '0' && symbol <= '9') value = (value * 10) + (symbol - '0');
                    else break;
                }
            }

            return value * sign;
        }

        [[nodiscard]] static std::optional<std::int32_t> dimension(Cursor& cursor, const semantics::Registers& registers, const Symbol count, const Symbol dimen) {
            if (!cursor.empty() && cursor.lookahead(0).symbol == dimen) {
                cursor.advance();
                const auto index = integer(cursor, registers, count);
                if (!index) return std::nullopt;
                return registers.fetch(semantics::Registers::Type::Dimension, static_cast<std::size_t>(*index));
            }

            const auto raw = integer(cursor, registers, count);
            if (!raw) return std::nullopt;

            std::int32_t base = *raw * scale;

            if (!cursor.empty() && cursor.lookahead(0).values == ".") {
                cursor.advance();
                if (!cursor.empty()) {
                    const Token token = cursor.advance();
                    std::int64_t part = 0;
                    std::int64_t div = 1;

                    for (const char symbol : token.values) {
                        if (symbol >= '0' && symbol <= '9') {
                            part = (part * 10) + (symbol - '0');
                            div *= 10;
                            if (div >= 100000000) break;
                        } else break;
                    }

                    if (div > 1) {
                        base += static_cast<std::int32_t>((part * scale) / div);
                    }
                }
            }

            if (cursor.empty()) return base;

            const Token token = cursor.advance();
            return unit(base, token.values);
        }

    private:
        [[nodiscard]] static constexpr std::int32_t unit(const std::int32_t base, const std::string_view text) noexcept {
            if (text.size() != 2) return base;

            switch (static_cast<std::uint16_t>(text[0]) << 8 | static_cast<std::uint16_t>(text[1])) {
                case 'p' << 8 | 't': return base;                                                                       // 1 pt
                case 'p' << 8 | 'c': return base * 12;                                                                  // 1 pc = 12 pt
                case 'i' << 8 | 'n': return static_cast<std::int32_t>(static_cast<std::int64_t>(base) * 7227 / 100);    // 1 in = 72.27 pt
                case 'b' << 8 | 'p': return static_cast<std::int32_t>(static_cast<std::int64_t>(base) * 7227 / 7200);   // 1 bp = 72.27/72 pt
                case 'c' << 8 | 'm': return static_cast<std::int32_t>(static_cast<std::int64_t>(base) * 7227 / 254);    // 1 cm = 72.27/2.54 pt
                case 'm' << 8 | 'm': return static_cast<std::int32_t>(static_cast<std::int64_t>(base) * 7227 / 2540);   // 1 mm = 72.27/25.4 pt
                case 's' << 8 | 'p': return base / scale;                                                               // 1 sp
                default: return base;
            }
        }
    };

}