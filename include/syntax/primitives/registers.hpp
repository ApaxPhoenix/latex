#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"

namespace syntax::primitives::registers {
    void ingest(Mouth& mouth, semantics::Registers& store);
}