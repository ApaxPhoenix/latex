#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"

namespace syntax::primitives::expansion {
    void ingest(Mouth& mouth, semantics::Registers& store);
}