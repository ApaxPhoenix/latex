#include "syntax/lexer.hpp"
#include "memory/arena.hpp"
#include <cassert>

int main() {
    memory::Arena arena;
    syntax::CatCodes catcodes;
    syntax::Lexicon lexicon(arena);
    constexpr std::string_view text = "\\documentclass{article}\n\\begin{document}\nMath $x+y$\n\\end{document}";

    syntax::Lexer lexer(text, catcodes, lexicon);
    assert(!lexer.empty());

    const syntax::Token head = lexer.advance();
    assert(head.category == syntax::CatCodes::Category::Escape);
    assert(head.values == "\\documentclass");

    const syntax::Token group = lexer.advance();
    assert(group.values == "{");

    const syntax::Token data = lexer.advance();
    assert(data.values == "a");

    return 0;
}