#include "syntax/mouth.hpp"
#include "semantics/union.hpp"
#include "memory/arena.hpp"
#include <cassert>

int main() {
    memory::Arena arena;
    syntax::semantics::Union state;
    syntax::Lexicon lexicon(arena);
    syntax::Cursor cursor;

    syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

    const syntax::Symbol symbol = lexicon.intern("macro");
    const syntax::Mouth::Macro macro{.active = true};
    mouth.define(symbol, macro);
    assert(mouth.lookup(symbol).has_value());

    mouth.ingest(R"(\def\section#1{\textbf{#1}}\section{Title}\par)");

    assert(mouth.step());

    const syntax::Token item = mouth.read();
    assert(!item.values.empty());

    return 0;
}