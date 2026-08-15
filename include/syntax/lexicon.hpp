#pragma once

#include "memory/arena.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

namespace syntax {

    using Symbol = std::uint32_t;
    inline constexpr Symbol kInvalidSymbol = 0;

    class Lexicon {
    public:
        struct Hash {
            using is_transparent = void;
            std::size_t operator()(const std::string_view sv) const noexcept {
                return std::hash<std::string_view>{}(sv);
            }
        };

        explicit Lexicon(memory::Arena& arena) : arena(arena) {
            names.emplace_back("");
        }

        Symbol intern(const std::string_view name) {
            if (name.empty()) return kInvalidSymbol;

            if (const auto found = lookup.find(name); found != lookup.end()) {
                return found->second;
            }

            const std::string_view copy = arena.copy(name);
            const auto symbol = static_cast<Symbol>(names.size());
            lookup.emplace(copy, symbol);
            names.push_back(copy);
            return symbol;
        }

        [[nodiscard]] std::string_view resolve(const Symbol symbol) const noexcept {
            if (symbol < names.size()) {
                return names[symbol];
            }
            return {};
        }

    private:
        memory::Arena& arena;
        std::unordered_map<std::string_view, Symbol, Hash, std::equal_to<>> lookup{};
        std::vector<std::string_view> names{};
    };

}