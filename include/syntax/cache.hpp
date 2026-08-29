#pragma once

#include "syntax/lexer.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace syntax {

    struct Cache {
        const Lexicon* lexicon = nullptr;
        std::array<std::pair<Symbol, std::string_view>, 256> entries{};

        Cache() {
            entries.fill({kInvalidSymbol, {}});
        }
    };

    inline thread_local Cache cache;

    inline std::pair<Symbol, std::string_view> entry(Lexicon& lexicon, const std::string_view slice) {
        if (slice.size() == 1) {
            const auto index = static_cast<unsigned char>(slice[0]);
            if (cache.lexicon != &lexicon) {
                cache.lexicon = &lexicon;
                cache.entries.fill({kInvalidSymbol, {}});
            }
            if (cache.entries[index].first == kInvalidSymbol) {
                const Symbol bound = lexicon.intern(slice);
                cache.entries[index] = {bound, lexicon.resolve(bound)};
            }
            return cache.entries[index];
        }
        const Symbol bound = lexicon.intern(slice);
        return {bound, lexicon.resolve(bound)};
    }

}