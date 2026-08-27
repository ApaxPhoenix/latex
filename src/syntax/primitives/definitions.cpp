#include "syntax/primitives/definitions.hpp"

namespace syntax::primitives::definitions {

    namespace {

        Mouth::Macro capture(Mouth& mouth, const bool expanded) {
            std::vector<Mouth::Parameter> parameters;
            std::vector<Token> pattern;

            while (true) {
                const Token token = mouth.read();
                if (token.category == CatCodes::Category::Group && token.values == "{") break;
                if (token.category == CatCodes::Category::Parameter) {
                    const Token digit = mouth.read();
                    pattern.push_back(digit);
                    parameters.emplace_back();
                    continue;
                }
                pattern.push_back(token);
            }

            std::vector<Token> body;
            std::size_t depth = 1;

            while (depth > 0) {
                const Token token = expanded ? mouth.expand() : mouth.read();
                if (token.values.empty()) break;

                if (token.category == CatCodes::Category::Group) {
                    if (token.values == "{") depth++;
                    else if (token.values == "}") {
                        depth--;
                        if (depth == 0) break;
                    }
                }
                body.push_back(token);
            }

            Mouth::Macro macro;
            macro.parameters = std::move(parameters);
            macro.body = std::move(body);
            return macro;
        }

        void assign(Mouth& mouth, const bool expanded, const bool global) {
            const Token name = mouth.read();
            if (name.category != CatCodes::Category::Escape) return;

            Mouth::Macro macro = capture(mouth, expanded);
            if (global) mouth.globalize();
            mouth.define(name.symbol, std::move(macro));
        }

        void alias(Mouth& mouth) {
            const Token name = mouth.read();
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            while (true) {
                const Token space = mouth.read();
                if (space.category != CatCodes::Category::Space) {
                    mouth.inject(std::span{&space, 1});
                    break;
                }
            }

            const Token target = mouth.read();
            if (const auto meaning = mouth.lookup(target.symbol)) {
                mouth.define(name.symbol, *meaning);
            } else if (const Mouth::Handler routine = mouth.primitive(target.symbol)) {
                mouth.bind(name.symbol, routine);
            }
        }

        void anticipate(Mouth& mouth) {
            const Token name = mouth.read();
            const Token target = mouth.lookahead(1);

            if (const auto meaning = mouth.lookup(target.symbol)) {
                mouth.define(name.symbol, *meaning);
            } else if (const Mouth::Handler routine = mouth.primitive(target.symbol)) {
                mouth.bind(name.symbol, routine);
            }
        }

    }

    void ingest(Mouth& mouth) {
        mouth.bind("\\def", [](Mouth& mouth) { assign(mouth, false, false); });
        mouth.bind("\\edef", [](Mouth& mouth) { assign(mouth, true, false); });
        mouth.bind("\\gdef", [](Mouth& mouth) { assign(mouth, false, true); });
        mouth.bind("\\xdef", [](Mouth& mouth) { assign(mouth, true, true); });
        mouth.bind("\\let", alias);
        mouth.bind("\\futurelet", anticipate);
    }

}