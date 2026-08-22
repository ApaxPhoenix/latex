#pragma once

#include "syntax/tokens.hpp"

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>

namespace syntax::semantics {

    class Registers {
    public:
        enum class Type : std::uint8_t {
            Count,     // Integer value register slots
            Dimension, // Fixed-point dimension distance slots
            Glue       // Flexible spacing distance slots
        };

        struct Target {
            Type type;
            std::size_t slot;
        };

        struct Entry {
            Type type;
            std::size_t slot;
            std::int32_t value;
        };

        void push();
        void pop();
        void assign(Type type, std::size_t index, std::int32_t value, bool global);
        [[nodiscard]] std::int32_t fetch(Type type, std::size_t index) const noexcept;
        void bind(syntax::Symbol symbol, Type type, std::size_t index);
        void set(syntax::Symbol symbol, std::int32_t value, bool global);
        [[nodiscard]] std::int32_t get(syntax::Symbol symbol) const noexcept;

    private:
        std::array<std::int32_t, 256> counts{};
        std::array<std::int32_t, 256> dimensions{};
        std::array<std::int32_t, 256> glues{};
        std::vector<Entry> entries{};
        std::vector<std::size_t> marks{};
        std::unordered_map<syntax::Symbol, Target> aliases{};
    };

}