#include "syntax/parser.hpp"
#include "syntax/mouth.hpp"
#include "semantics/union.hpp"
#include "memory/arena.hpp"
#include "modules.hpp"

#include <cassert>

int main() {
    memory::Arena arena;
    syntax::semantics::Union state;
    syntax::Lexicon lexicon(arena);

    syntax::Cursor cursor;
    syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

    mouth.ingest("\\begin{document}\nWelcome to LaTeX.\\par\n\\end{document}");

    const auto core = syntax::modules::find("main.mtex");
    assert(core);
    mouth.ingest(*core);

    syntax::Parser parser(mouth, arena);
    const memory::Slice<syntax::Node*> nodes = parser.parse();

    assert(!nodes.empty());

    return 0;
}