#pragma once

#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/tokens.hpp"
#include "syntax/traceback.hpp"
#include "syntax/semantics/union.hpp"
#include "memory/arena.hpp"

#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace syntax {

    class Mouth {
    public:
        using Handler = std::function<void(Mouth&)>;

        struct Parameter {
            bool optional = false;
            std::vector<Token> fallbacks{};
            std::vector<Token> delimiters{};
        };

        struct Macro {
            bool active = false;
            std::vector<Parameter> parameters{};
            std::vector<Token> body{};
        };

        struct Record {
            Symbol symbol = kInvalidSymbol;
            Macro macros{};
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
        void inhibit() noexcept { this->suppress = true; }

        [[nodiscard]] semantics::Union& state() const noexcept { return union_; }
        [[nodiscard]] Lexicon& lexicon() const noexcept { return lexicon_; }
        [[nodiscard]] std::vector<Traceback>& tracebacks() noexcept { return tracebacks_; }
        [[nodiscard]] const std::vector<Traceback>& tracebacks() const noexcept { return tracebacks_; }

    private:
        Cursor cursor;
        semantics::Union& union_;
        Lexicon& lexicon_;
        memory::Arena& arena;

        std::vector<Handler> handler{};
        std::vector<Macro> macros{};
        std::vector<Record> records{};
        std::vector<std::size_t> marks{};
        std::vector<Traceback> tracebacks_{};

        bool suppress = false;
        bool global = false;
        std::size_t depth = 0;
        std::size_t limit = 10000;
    };

}