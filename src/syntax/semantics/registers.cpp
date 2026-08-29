#include "syntax/semantics/registers.hpp"
#include "logger.hpp"

#include <utility>

namespace syntax::semantics {

    void Registers::push() {
        marks.push_back(Mark{entries.size(), records.size()});
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Pushed register scope checkpoint at entries index {} tokens index {}", entries.size(), records.size());
    }

    void Registers::pop() {
        if (marks.empty()) {
            Logger::log(Logger::Type::Semantics, Logger::Level::Warning,
                        "Attempted to pop register checkpoint on empty stack");
            return;
        }
        const auto mark = marks.back();
        marks.pop_back();

        using enum Type;
        const auto count = entries.size() - mark.entries;
        for (std::size_t index = 0uz; index < count; ++index) {
            const auto [type, slot, value] = entries.back();
            entries.pop_back();

            switch (type) {
                case Count:     counts[slot] = value; break;
                case Dimension: dimensions[slot] = value; break;
                case Glue:      glues[slot] = value; break;
                case Tokens:    break; // token registers are restored from records below
            }
        }

        const auto tokenCount = records.size() - mark.tokens;
        for (std::size_t index = 0uz; index < tokenCount; ++index) {
            auto [slot, value] = std::move(records.back());
            records.pop_back();
            tokens[slot] = std::move(value);
        }

        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Restored register state back to checkpoint entries {} tokens {}", mark.entries, mark.tokens);
    }

    void Registers::assign(const Type type, const std::size_t index, const std::int32_t value, const bool global) {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Register index {} out of bounds (maximum 255)", index);
            return;
        }

        if (type == Type::Tokens) {
            Logger::log(Logger::Type::Semantics, Logger::Level::Warning,
                        "Token registers cannot take a scalar value; use the vector-typed assign overload");
            return;
        }

        if (!global && !marks.empty()) {
            entries.push_back({type, index, fetch(type, index)});
        }

        using enum Type;
        switch (type) {
            case Count:     counts[index] = value; break;
            case Dimension: dimensions[index] = value; break;
            case Glue:      glues[index] = value; break;
            case Tokens:    break; // unreachable, guarded above
        }

        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Assigned register slot {} to value {} (global: {})", index, value, global);
    }

    void Registers::assign(const std::size_t index, std::vector<Token> value, const bool global) {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Token register index {} out of bounds (maximum 255)", index);
            return;
        }

        if (!global && !marks.empty()) {
            records.push_back(Record{index, tokens[index]});
        }

        tokens[index] = std::move(value);

        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Assigned token register slot {} ({} token(s), global: {})", index, tokens[index].size(), global);
    }

    std::int32_t Registers::fetch(const Type type, const std::size_t index) const noexcept {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Fetch operation requested out-of-bounds register slot {}", index);
            return 0;
        }

        using enum Type;
        switch (type) {
            case Count:     return counts[index];
            case Dimension: return dimensions[index];
            case Glue:      return glues[index];
            case Tokens:
                Logger::log(Logger::Type::Semantics, Logger::Level::Warning,
                            "Token registers have no scalar value; use the vector-typed fetch overload");
                return 0;
        }
        return 0;
    }

    const std::vector<Token>& Registers::get(const std::size_t index) const noexcept {
        static const std::vector<Token> empty{};
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Fetch operation requested out-of-bounds token register slot {}", index);
            return empty;
        }
        return tokens[index];
    }

    void Registers::bind(const Symbol symbol, const Type type, const std::size_t index) {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Cannot bind text {} to out-of-bounds slot {}", symbol, index);
            return;
        }
        aliases[symbol] = Target{type, index};
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Bound text alias {} to register slot {}", symbol, index);
    }

    void Registers::set(const Symbol symbol, const std::int32_t value, const bool global) {
        const auto match = aliases.find(symbol);
        if (match == aliases.end()) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Warning,
                        "Attempted set operation on unbound text alias {}", symbol);
            return;
        }
        assign(match->second.type, match->second.slot, value, global);
    }

    std::int32_t Registers::get(const Symbol symbol) const noexcept {
        const auto match = aliases.find(symbol);
        if (match == aliases.end()) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                        "Symbol {} not bound to any register target", symbol);
            return 0;
        }
        return fetch(match->second.type, match->second.slot);
    }

}