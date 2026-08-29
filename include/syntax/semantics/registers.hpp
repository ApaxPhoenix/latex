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
            Glue,      // Flexible spacing distance slots
            Tokens     // Raw token-list register slots (\toks)
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

        // \toks stores a whole token sequence, not a scalar, so its undo
        // log needs its own entry shape instead of reusing Entry.
        struct Record {
            std::size_t slot;
            std::vector<Token> value;
        };

        // A scope checkpoint has to remember how far into *both* undo logs
        // it was taken, since scalar and token-list assignments each keep
        // a separate log.
        struct Mark {
            std::size_t entries;
            std::size_t tokens;
        };

        void push();
        void pop();

        void assign(Type type, std::size_t index, std::int32_t value, bool global);
        [[nodiscard]] std::int32_t fetch(Type type, std::size_t index) const noexcept;

        void assign(std::size_t index, std::vector<Token> value, bool global);
        [[nodiscard]] const std::vector<Token>& get(std::size_t index) const noexcept;

        void bind(Symbol symbol, Type type, std::size_t index);
        void set(Symbol symbol, std::int32_t value, bool global);
        [[nodiscard]] std::int32_t get(Symbol symbol) const noexcept;

    private:
        std::array<std::int32_t, 256> counts{};
        std::array<std::int32_t, 256> dimensions{};
        std::array<std::int32_t, 256> glues{};
        std::array<std::vector<Token>, 256> tokens{};
        std::vector<Entry> entries{};
        std::vector<Record> records{};
        std::vector<Mark> marks{};
        std::unordered_map<Symbol, Target> aliases{};
    };

}