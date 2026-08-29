#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"

namespace syntax::primitives::conditionals {

    class Gate {
    public:
        explicit Gate(Lexicon& lexicon) noexcept;
        void ingest(Mouth& mouth, semantics::Registers& registers) const;
        void path(Mouth& mouth, bool condition) const noexcept;

    private:
        void skip(Mouth& mouth) const noexcept;
        void drop(Mouth& mouth) const noexcept;
        static bool compare(const Mouth::Macro& first, const Mouth::Macro& second) noexcept;

        Symbol finish{};
        Symbol alternative{};
        Symbol branch{};
        Symbol identifier{};
        Symbol dimension{};
        Symbol terminate{};

        mutable bool inverted = false;
    };

}