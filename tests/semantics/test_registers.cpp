#include "syntax/semantics/registers.hpp"
#include <cassert>

int main() {
    syntax::semantics::Registers registers;

    registers.assign(syntax::semantics::Registers::Type::Count, 0, 42, false);
    assert(registers.fetch(syntax::semantics::Registers::Type::Count, 0) == 42);

    registers.push();
    registers.assign(syntax::semantics::Registers::Type::Count, 0, 100, false);
    assert(registers.fetch(syntax::semantics::Registers::Type::Count, 0) == 100);

    registers.pop();
    assert(registers.fetch(syntax::semantics::Registers::Type::Count, 0) == 42);

    return 0;
}