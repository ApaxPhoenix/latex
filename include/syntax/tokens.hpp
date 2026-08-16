#pragma once

#include "memory/location.hpp"
#include "syntax/catcodes.hpp"
#include "syntax/lexicon.hpp"

#include <string_view>

namespace syntax {

    struct Token {
        Symbol symbol = kInvalidSymbol;
        std::string_view values{};
        memory::Location location{};
        CatCodes::Category category = CatCodes::Category::Other;
    };

}
