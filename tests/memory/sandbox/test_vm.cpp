#include "vm.hpp"

#include <cassert>

int main() {
    constexpr sandbox::Policy policy{
        .shell = false,
        .write = false,
        .read = true,
        .tokens = 100
    };

    sandbox::VM vm(policy, 1024 * 1024);
    assert(vm.eval("\\documentclass{article}\n\\begin{document}\nHello\n\\end{document}"));

    return 0;
}