#pragma once

#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/tokens.hpp"
#include "syntax/traceback.hpp"
#include "memory/arena.hpp"

#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace syntax {

    namespace semantics {
        class Union;
        class Registers;
    }

    class Mouth {
    public:
        using Handler = std::function<void(Mouth&)>;

        struct Parameter {
            bool optional = false;
            std::vector<Token> delimiters{};
            std::vector<Token> fallbacks{};
        };

        struct Macro {
            std::vector<Parameter> parameters{};
            std::vector<Token> body{};
            bool active = false;
            bool spanning = false;
            bool isolated = false;
        };

        struct Record {
            Symbol symbol = kInvalidSymbol;
            Macro macro{};
            bool active = false;
        };

        Mouth(Cursor input_cursor, semantics::Union& state, Lexicon& lexicon, memory::Arena& arena);

        [[nodiscard]] Lexicon& lexicon() noexcept { return lexicon_; }
        [[nodiscard]] const Lexicon& lexicon() const noexcept { return lexicon_; }

        [[nodiscard]] semantics::Union& state() noexcept { return state_; }
        [[nodiscard]] const semantics::Union& state() const noexcept { return state_; }

        [[nodiscard]] memory::Arena& arena() const noexcept { return arena_; }

        void push();
        void pop();

        void globalize() noexcept;
        int unglobal() noexcept;

        void longify() noexcept;
        int unlong() noexcept;

        void outerize() noexcept;
        int unouter() noexcept;

        void defer(const Token& token);
        void schedule(const Token& token);

        Token read() noexcept;
        Token expand();
        int step();

        std::vector<Token> argument(const Parameter& parameter, int allow);
        void inject(std::span<const Token> tokens);
        void ingest(std::string_view source);

        void bind(std::string_view name, Handler handler);
        void bind(Symbol symbol, Handler handler);

        void define(std::string_view name, Macro macro);
        void define(Symbol symbol, Macro macro);

        void undefine(std::string_view name);
        void undefine(Symbol symbol);

        [[nodiscard]] Token lookahead(std::size_t offset = 0) const noexcept;
        [[nodiscard]] std::optional<Macro> lookup(Symbol symbol) const noexcept;
        [[nodiscard]] Handler primitive(Symbol symbol) const noexcept;
        [[nodiscard]] const std::vector<Traceback>& history() const noexcept { return history_; }

        std::optional<std::int32_t> integer(const semantics::Registers& registers, Symbol count);
        std::optional<std::int32_t> dimension(const semantics::Registers& registers, Symbol count, Symbol unit);

    private:
        Cursor cursor{};
        semantics::Union& state_;
        Lexicon& lexicon_;
        memory::Arena& arena_;

        Symbol paragraph = kInvalidSymbol;

        std::vector<std::size_t> marks{};
        std::vector<std::vector<Token>> deferred{};
        std::vector<Traceback> history_{};
        std::vector<Record> records{};

        std::vector<Macro> macros{};
        std::vector<Handler> handlers{};

        int global = 0;
        int spanning = 0;
        int isolated = 0;
        int suppressed = 0;

        std::optional<Token> scheduled{};

        std::size_t depth = 0;
        std::size_t limit = 256;
    };

}