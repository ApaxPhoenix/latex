#pragma once

#include "syntax/parser.hpp"
#include "syntax/semantics/registers.hpp"

namespace render::primitives::glue {
    void ingest(syntax::Parser& parser, syntax::semantics::Registers& registers);
}