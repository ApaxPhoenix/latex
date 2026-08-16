#pragma once

#include "layout/breaker.hpp"
#include "layout/line.hpp"
#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "semantics/union.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/node.hpp"
#include "typography/suite.hpp"

namespace layout {

    class Typesetter {
    public:
        Typesetter(memory::Arena& arena, typography::Suite& suite, semantics::Union& state, syntax::Lexicon& lexicon) noexcept;

        [[nodiscard]] Node* compose(memory::Slice<syntax::Node*> input) const;

    private:
        memory::Arena& arena;
        typography::Suite& suite;
        semantics::Union& state;
        syntax::Lexicon& lexicon;
    };

}