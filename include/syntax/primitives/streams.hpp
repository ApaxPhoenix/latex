#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"
#include "conditionals.hpp"

namespace syntax::primitives::streams {
    void ingest(Mouth& mouth, semantics::Registers& registers, const conditionals::Gate& gate);
}