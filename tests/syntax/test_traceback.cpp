#include "syntax/traceback.hpp"
#include <cassert>

int main() {
    constexpr memory::Location location{.line = 14, .column = 2};
    const syntax::Traceback traceback(syntax::Traceback::Type::Macro, location, "Undefined control sequence \\unknown");

    const std::string text = traceback.format();

    assert(text.find("14:2") != std::string::npos);
    assert(text.find("\\unknown") != std::string::npos);

    return 0;
}