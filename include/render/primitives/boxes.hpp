#pragma once

#include "syntax/parser.hpp"
#include "syntax/semantics/registers.hpp"
#include "render/layout/typesetter.hpp"
#include "render/primitives/fonts.hpp"
#include "typography/shaper.hpp"

namespace render::primitives::boxes {

    void ingest(
        syntax::Parser& parser,
        syntax::semantics::Registers& registers,
        const typography::Shaper& shaper,
        const layout::Typesetter& typesetter,
        const fonts::Selection& selection
    );

}