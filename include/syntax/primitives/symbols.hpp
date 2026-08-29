#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"
#include "syntax/expression/unicodes.hpp"

namespace syntax::primitives::symbols {
    void ingest(Mouth& mouth, expression::Unicodes& glyphs, semantics::Registers& registers);
}