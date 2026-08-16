#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace expression {

    class Unicodes {
    public:
        enum class Type : std::uint8_t {
            Ordinary,
            Operator,
            Binary,
            Relation,
            Opening,
            Closing,
            Punctuation,
            Inner,
            Accent
        };

        struct Symbol {
            std::uint32_t codepoint;
            Type type;
        };

        struct Entry {
            const char* name;
            std::uint32_t codepoint;
            Type type;
        };

        static const std::uint32_t invalid;

        Unicodes();

        void compose(std::string_view name, std::uint32_t codepoint, Type type);
        void dispose(std::string_view name);
        [[nodiscard]] std::optional<Symbol> query(std::string_view name) const noexcept;

    private:
        struct Hash {
            using is_transparent = void;
            std::size_t operator()(const std::string_view value) const noexcept {
                return std::hash<std::string_view>{}(value);
            }
        };

        std::unordered_map<std::string, Symbol, Hash, std::equal_to<>> overrides;
    };

}