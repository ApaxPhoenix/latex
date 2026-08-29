#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"

namespace syntax::primitives::characters {
    void ingest(Mouth& mouth, semantics::Registers& registers);
}