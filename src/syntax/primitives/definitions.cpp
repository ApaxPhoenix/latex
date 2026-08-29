#include "syntax/primitives/definitions.hpp"
#include "logger.hpp"

#include <span>
#include <utility>

namespace syntax::primitives::definitions {

    void ingest(Mouth& mouth) {
        static auto define = [](Mouth& mouth, const bool expand, const bool global) {
            const bool spanning = mouth.unlong();
            const bool isolated = mouth.unouter();
            const Token token = mouth.read();

            if (token.category != CatCodes::Category::Escape && token.category != CatCodes::Category::Active) return;

            std::vector<Mouth::Parameter> arguments;
            std::vector<Token> pending;

            while (true) {
                const Token item = mouth.read();

                if (item.category == CatCodes::Category::Group && item.values == "{") {
                    if (!arguments.empty()) arguments.back().delimiters = std::move(pending);
                    break;
                }

                if (item.category == CatCodes::Category::Parameter) {
                    mouth.read();
                    if (!arguments.empty()) arguments.back().delimiters = std::move(pending);
                    pending.clear();
                    arguments.emplace_back();
                    continue;
                }
                pending.push_back(item);
            }

            std::vector<Token> body;
            std::size_t depth = 1uz;

            while (depth > 0uz) {
                const Token item = expand ? mouth.expand() : mouth.read();
                if (item.values.empty()) break;

                if (item.category == CatCodes::Category::Group) {
                    if (item.values == "{") depth++;
                    else if (item.values == "}") {
                        depth--;
                        if (depth == 0uz) break;
                    }
                }
                body.push_back(item);
            }

            Mouth::Macro macro;
            macro.parameters = std::move(arguments);
            macro.body = std::move(body);
            macro.spanning = spanning;
            macro.isolated = isolated;

            if (global) mouth.globalize();
            mouth.define(token.symbol, std::move(macro));
        };

        mouth.bind("\\def", [](Mouth& mouth) { define(mouth, false, false); });
        mouth.bind("\\edef", [](Mouth& mouth) { define(mouth, true, false); });
        mouth.bind("\\gdef", [](Mouth& mouth) { define(mouth, false, true); });
        mouth.bind("\\xdef", [](Mouth& mouth) { define(mouth, true, true); });

        mouth.bind("\\let", [](Mouth& mouth) {
            const Token token = mouth.read();
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            while (true) {
                if (const Token space = mouth.read(); space.category != CatCodes::Category::Space) {
                    mouth.inject(std::span{&space, 1});
                    break;
                }
            }

            const Token reference = mouth.read();
            if (const auto macro = mouth.lookup(reference.symbol)) mouth.define(token.symbol, *macro);
            else if (const Mouth::Handler handler = mouth.primitive(reference.symbol)) mouth.bind(token.symbol, handler);
        });

        mouth.bind("\\futurelet", [](Mouth& mouth) {
            const Token token = mouth.read();
            const Token reference = mouth.lookahead(1);

            if (const auto macro = mouth.lookup(reference.symbol)) mouth.define(token.symbol, *macro);
            else if (const Mouth::Handler handler = mouth.primitive(reference.symbol)) mouth.bind(token.symbol, handler);
        });

        mouth.bind("\\undef", [](Mouth& mouth) {
            mouth.undefine(mouth.read().symbol);
        });

        mouth.bind("\\global", [](Mouth& mouth) { mouth.globalize(); });
        mouth.bind("\\long", [](Mouth& mouth) { mouth.longify(); });
        mouth.bind("\\outer", [](Mouth& mouth) { mouth.outerize(); });
    }

}