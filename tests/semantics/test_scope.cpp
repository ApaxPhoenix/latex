#include "syntax/semantics/scope.hpp"
#include <cassert>

int main() {
    syntax::semantics::Scope scope;

    assert(scope.depth() == 0);
    scope.push(syntax::semantics::Scope::Type::Group);
    assert(scope.depth() == 1);
    scope.pop();
    assert(scope.depth() == 0);

    return 0;
}