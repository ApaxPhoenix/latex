#pragma once

#include "layout/document.hpp"
#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"

namespace render::primitives::document {

    void ingest(syntax::Mouth& mouth, layout::Document& document, syntax::semantics::Registers& registers);

}