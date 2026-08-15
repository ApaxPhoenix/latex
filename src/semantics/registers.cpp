#include "semantics/registers.hpp"
#include "logger.hpp"

#include <utility>

namespace semantics {

    void Registers::push() {
        marks.push_back(undo.size());
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Pushed register scope checkpoint at undo index {}", undo.size());
    }

    void Registers::pop() {
        if (marks.empty()) {
            Logger::log(Logger::Type::Semantics, Logger::Level::Warn,
                        "Attempted to pop register checkpoint on empty stack");
            return;
        }
        const auto mark = marks.back();
        marks.pop_back();

        using enum Type;
        const auto count = undo.size() - mark;
        for (std::size_t index = 0uz; index < count; ++index) {
            const auto [type, slot, value] = undo.back();
            undo.pop_back();

            switch (type) {
                case Count:     counts[slot] = value; break;
                case Dimension: dimensions[slot] = value; break;
                case Glue:      glues[slot] = value; break;
            }
        }

        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Restored register state back to checkpoint index {}", mark);
    }

    void Registers::assign(const Type type, const std::size_t index, const std::int32_t value, const bool global) {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Register index {} out of bounds (maximum 255)", index);
            return;
        }

        if (!global && !marks.empty()) {
            undo.push_back({type, index, fetch(type, index)});
        }

        using enum Type;
        switch (type) {
            case Count:     counts[index] = value; break;
            case Dimension: dimensions[index] = value; break;
            case Glue:      glues[index] = value; break;
        }

        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Assigned register slot {} to value {} (global: {})", index, value, global);
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
        }
        return 0;
    }

    void Registers::bind(const syntax::Symbol symbol, const Type type, const std::size_t index) {
        if (index >= 256uz) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Error,
                        "Cannot bind letters {} to out-of-bounds slot {}", symbol, index);
            return;
        }
        aliases[symbol] = Target{type, index};
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Bound letters alias {} to register slot {}", symbol, index);
    }

    void Registers::set(const syntax::Symbol symbol, const std::int32_t value, const bool global) {
        const auto match = aliases.find(symbol);
        if (match == aliases.end()) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Warn,
                        "Attempted set operation on unbound letters alias {}", symbol);
            return;
        }
        assign(match->second.type, match->second.slot, value, global);
    }

    std::int32_t Registers::get(const syntax::Symbol symbol) const noexcept {
        const auto match = aliases.find(symbol);
        if (match == aliases.end()) {
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                        "Symbol {} not bound to any register target", symbol);
            return 0;
        }
        return fetch(match->second.type, match->second.slot);
    }

}