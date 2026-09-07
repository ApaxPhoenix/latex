#pragma once

#include "syntax/expression/parser.hpp"
#include "syntax/expression/unicodes.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/parser.hpp"

namespace render::primitives::expression {

    void rules(syntax::expression::Parser& parser, syntax::Lexicon& lexicon);
    void ingest(syntax::Parser& parser, const syntax::expression::Unicodes& unicodes);

}