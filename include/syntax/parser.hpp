#pragma once

#include "syntax/ast.hpp"
#include "syntax/mouth.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/traceback.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <functional>
#include <string_view>
#include <vector>

namespace syntax {

    class Parser {
    public:
        using Handler = std::function<Node*(Parser&)>;

        Parser(Mouth& mouth, memory::Arena& arena);

        [[nodiscard]] Token step() const;
        [[nodiscard]] memory::Slice<Node*> parse();

        void bind(std::string_view name, Handler handler);
        void bind(Symbol symbol, Handler handler);

        [[nodiscard]] Mouth& mouth() const noexcept { return mouth_; }
        [[nodiscard]] memory::Arena& arena() const noexcept { return arena_; }
        [[nodiscard]] const std::vector<Traceback>& trace() const noexcept { return tracebacks_; }

    private:
        Mouth& mouth_;
        memory::Arena& arena_;
        std::vector<Handler> handlers_{};
        std::vector<Traceback> tracebacks_{};
    };

}