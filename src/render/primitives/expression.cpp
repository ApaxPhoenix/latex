#include "render/primitives/expression.hpp"

#include "syntax/mouth.hpp"
#include "syntax/node.hpp"
#include "syntax/tokens.hpp"

#include <span>

namespace render::primitives::expression {

    void rules(syntax::expression::Parser& parser, syntax::Lexicon& lexicon) {
        using Type = syntax::expression::Node::Type;

        const auto bind = [&](const std::string_view name, const Type type, const int weight = 0,
                               const bool right = false, const bool structural = false) {
            parser.bind(lexicon.intern(name), type, weight, right, structural);
        };

        bind("=", Type::Binary, 1);
        bind("<", Type::Binary, 1);
        bind(">", Type::Binary, 1);
        bind("\\le", Type::Binary, 1);
        bind("\\ge", Type::Binary, 1);
        bind("\\neq", Type::Binary, 1);
        bind("\\equiv", Type::Binary, 1);

        bind("+", Type::Binary, 2);
        bind("-", Type::Binary, 2);
        bind("\\pm", Type::Binary, 2);
        bind("\\mp", Type::Binary, 2);

        bind("*", Type::Binary, 3);
        bind("/", Type::Binary, 3);
        bind("\\times", Type::Binary, 3);
        bind("\\cdot", Type::Binary, 3);
        bind("\\div", Type::Binary, 3);

        bind("!", Type::Unary, 10);

        bind("\\frac", Type::Fraction, 0, false, true);
        bind("\\sqrt", Type::Radical, 0, false, true);
        bind("\\hat", Type::Accent, 0, false, true);
        bind("\\vec", Type::Accent, 0, false, true);
        bind("\\bar", Type::Accent, 0, false, true);
        bind("\\dot", Type::Accent, 0, false, true);
        bind("\\ddot", Type::Accent, 0, false, true);
        bind("\\tilde", Type::Accent, 0, false, true);
        bind("\\breve", Type::Accent, 0, false, true);
        bind("\\check", Type::Accent, 0, false, true);
        bind("\\acute", Type::Accent, 0, false, true);
        bind("\\grave", Type::Accent, 0, false, true);
    }

    namespace {

        syntax::Node* enter(
            syntax::Parser& parser,
            const syntax::expression::Unicodes& unicodes,
            const bool display,
            const char delimiter,
            const syntax::Symbol stop
        ) {
            syntax::Mouth& mouth = parser.mouth();
            memory::Arena& arena = parser.arena();
            const memory::Location origin = mouth.lookahead().location;

            syntax::expression::Parser inner(
                mouth, unicodes, arena,
                display ? syntax::expression::Node::Style::Display
                        : syntax::expression::Node::Style::Inline
            );
            rules(inner, mouth.lexicon());

            syntax::expression::Node* tree = delimiter != 0 ? inner.parse(delimiter) : inner.parse(stop);
            syntax::Token leftover = inner.pending();

            if (display && delimiter == '$' && leftover.values == "$") {
                leftover = syntax::Token{};
            }

            if (!leftover.empty()) {
                mouth.inject(std::span{&leftover, 1});
            }

            auto* node = arena.compose<syntax::Node>(syntax::Node::Type::Expression, std::string_view{}, origin);
            node->expression = tree;
            return node;
        }

    }

    void ingest(syntax::Parser& parser, const syntax::expression::Unicodes& unicodes) {
        parser.bind("$", [&unicodes](syntax::Parser& parser) -> syntax::Node* {
            syntax::Mouth& mouth = parser.mouth();
            syntax::Token next = mouth.read();
            bool display = false;

            if (next.values == "$") {
                display = true;
            } else if (!next.empty()) {
                mouth.inject(std::span{&next, 1});
            }

            return enter(parser, unicodes, display, '$', syntax::kInvalidSymbol);
        });

        parser.bind("\\(", [&unicodes](syntax::Parser& parser) -> syntax::Node* {
            const syntax::Symbol stop = parser.mouth().lexicon().intern("\\)");
            return enter(parser, unicodes, false, 0, stop);
        });

        parser.bind("\\[", [&unicodes](syntax::Parser& parser) -> syntax::Node* {
            const syntax::Symbol stop = parser.mouth().lexicon().intern("\\]");
            return enter(parser, unicodes, true, 0, stop);
        });
    }

}