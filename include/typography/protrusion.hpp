#pragma once

#include <cstdint>
#include <unordered_map>

namespace typography {

    struct Margin {
        std::int32_t left{0};
        std::int32_t right{0};
    };

    class Protrusion {
    public:
        void assign(const char symbol, const std::int32_t left, const std::int32_t right) {
            table[symbol] = {left, right};
        }

        [[nodiscard]] Margin fetch(const char symbol) const noexcept {
            if (const auto iterator = table.find(symbol); iterator != table.end()) {
                return iterator->second;
            }
            return {0, 0};
        }

    private:
        std::unordered_map<char, Margin> table{
                {',', {0, 700}},
                {'.', {0, 700}},
                {'-', {0, 500}},
                {';', {0, 500}},
                {':', {0, 500}},
                {'`', {700, 0}},
                {'\'', {0, 700}}
        };
    };

}