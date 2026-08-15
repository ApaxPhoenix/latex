#pragma once

#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/tokens.hpp"
#include "syntax/traceback.hpp"
#include "semantics/union.hpp"
#include "memory/arena.hpp"

#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace syntax {

    class Mouth {
    public:
        using Handler = std::function<void(Mouth&)>;

        struct Parameter {
            bool optional = false;
            std::vector<Token> fallback{};
            std::vector<Token> delimiter{};
        };

        struct Macro {
            bool active = false;
            std::vector<Parameter> parameters{};
            std::vector<Token> body{};
        };

        struct Record {
            Symbol symbol = kInvalidSymbol;
            Macro macro{};
            bool active = false;
        };

        Mouth(Cursor cursor, semantics::Union& state, Lexicon& lexicon, memory::Arena& arena);

        Token read() noexcept;
        Token expand();
        bool step();
        std::vector<Token> argument(const Parameter& parameter);

        void push();
        void pop();
        void globalize() noexcept;

        void inject(std::span<const Token> tokens);
        void ingest(std::string_view source);
        void bind(std::string_view name, Handler handler);
        void bind(Symbol symbol, Handler handler);
        void define(std::string_view name, Macro macro);
        void define(Symbol symbol, Macro macro);
        void undefine(std::string_view name);
        void undefine(Symbol symbol);

        [[nodiscard]] semantics::Union& state() const noexcept { return state_; }
        [[nodiscard]] Lexicon& lexicon() const noexcept { return lexicon_; }
        [[nodiscard]] std::vector<Traceback>& tracebacks() noexcept { return tracebacks_; }
        [[nodiscard]] const std::vector<Traceback>& tracebacks() const noexcept { return tracebacks_; }

    private:
        Cursor cursor_;
        semantics::Union& state_;
        Lexicon& lexicon_;
        memory::Arena& arena_;

        Symbol count_{};
        Symbol dimension_{};
        Symbol else_{};
        Symbol fi_{};

        std::vector<Handler> primitives_{};
        std::vector<Macro> macros_{};
        std::vector<Record> undo_{};
        std::vector<std::size_t> marks_{};
        std::vector<Traceback> tracebacks_{};

        bool suppress_ = false;
        bool global_ = false;
        std::size_t depth_ = 0;
        std::size_t limit_ = 10000;
    };

}