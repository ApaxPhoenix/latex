#include "syntax/primitives/characters.hpp"
#include "syntax/primitives/casecodes.hpp"
#include "syntax/semantics/union.hpp"
#include "logger.hpp"

#include <span>

namespace syntax::primitives::characters {

    void ingest(Mouth& mouth, semantics::Registers& registers) {
        const Symbol identifier = mouth.lexicon().intern("\\count");
        static casecodes::Cases cases;

        mouth.bind("\\catcode", [&registers, identifier](Mouth& mouth) {
            const Token token = mouth.read();
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            if (const auto value = mouth.integer(registers, identifier); value && !token.values.empty()) {
                mouth.state().catcodes().set(token.values[0], static_cast<CatCodes::Category>(*value), false);
            }
        });

        mouth.bind("\\char", [&registers, identifier](Mouth& mouth) {
            if (const auto value = mouth.integer(registers, identifier)) {
                mouth.ingest(std::string(1, static_cast<char>(*value)));
            }
        });

        mouth.bind("\\uccode", [&registers, identifier](Mouth& mouth) {
            const Token token = mouth.read();
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            if (const auto value = mouth.integer(registers, identifier); value && !token.values.empty()) {
                cases.upper(token.values[0], static_cast<char>(*value));
            }
        });

        mouth.bind("\\lccode", [&registers, identifier](Mouth& mouth) {
            const Token token = mouth.read();
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            if (const auto value = mouth.integer(registers, identifier); value && !token.values.empty()) {
                cases.lower(token.values[0], static_cast<char>(*value));
            }
        });

        mouth.bind("\\lowercase", [](Mouth& mouth) {
            const Token token = mouth.read();
            std::string content(token.values);
            for (char& character : content) character = cases.lower(character);
            mouth.ingest(std::move(content));
        });

        mouth.bind("\\uppercase", [](Mouth& mouth) {
            const Token token = mouth.read();
            std::string content(token.values);
            for (char& character : content) character = cases.upper(character);
            mouth.ingest(std::move(content));
        });
    }

}