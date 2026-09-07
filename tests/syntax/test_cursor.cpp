#include "syntax/cursor.hpp"
#include <cassert>

int main() {
    constexpr syntax::Token first{.symbol = 1, .category = syntax::CatCodes::Category::Escape, .values = "\\begin"};
    constexpr syntax::Token second{.symbol = 2, .category = syntax::CatCodes::Category::Letter, .values = "document"};

    syntax::Cursor cursor({first, second});
    assert(!cursor.empty());

    assert(cursor.lookahead(0).values == "\\begin");
    assert(cursor.lookahead(1).values == "document");

    assert(cursor.advance().values == "\\begin");

    syntax::Token injected{.symbol = 3, .category = syntax::CatCodes::Category::Escape, .values = "\\end"};
    cursor.inject(std::span<const syntax::Token>(&injected, 1));
    assert(cursor.lookahead(0).values == "\\end");

    return 0;
}