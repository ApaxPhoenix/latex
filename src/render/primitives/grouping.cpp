#include "syntax/primitives/grouping.hpp"
#include "syntax/semantics/union.hpp"

namespace syntax::primitives::grouping {

    void ingest(Mouth& mouth) {
        mouth.bind("\\begingroup", [](Mouth& mouth) {
            mouth.state().push(semantics::Scope::Type::Group);
        });

        mouth.bind("\\endgroup", [](Mouth& mouth) {
            mouth.state().pop();
        });

        mouth.bind("\\bgroup", [](Mouth& mouth) {
            mouth.state().push(semantics::Scope::Type::Group);
        });

        mouth.bind("\\egroup", [](Mouth& mouth) {
            mouth.state().pop();
        });
    }

}