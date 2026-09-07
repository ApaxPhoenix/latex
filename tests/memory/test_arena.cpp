#include "memory/arena.hpp"

#include <cassert>
#include <string_view>

int main() {
    memory::Arena arena(1024);

    constexpr std::string_view text = "\\documentclass{article}\n\\begin{document}\nHello\n\\end{document}";
    const std::string_view copy = arena.copy(text);

    assert(copy == text);
    assert(copy.data() != text.data());

    const auto slice = arena.allocate<int>(4);
    assert(slice.size() == 4);

    return 0;
}