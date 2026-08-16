#include "layout/typesetter.hpp"
#include "layout/expression.hpp"
#include "expression/parser.hpp"
#include "expression/unicodes.hpp"
#include "syntax/cursor.hpp"
#include "syntax/mouth.hpp"
#include "typography/shaper.hpp"

#include <algorithm>
#include <vector>

namespace layout {

    Typesetter::Typesetter(memory::Arena& arena, typography::Suite& suite, semantics::Union& state, syntax::Lexicon& lexicon) noexcept
        : arena(arena), suite(suite), state(state), lexicon(lexicon) {}

    Node* Typesetter::compose(memory::Slice<syntax::Node*> input) const {
        if (input.empty()) return nullptr;

        const double width = state.registers().fetch(semantics::Registers::Type::Dimension, 0);
        const double skip = state.registers().fetch(semantics::Registers::Type::Dimension, 1);
        const double limit = state.registers().fetch(semantics::Registers::Type::Dimension, 2);

        const double target = width > 0.0 ? width : 400.0;
        const double leading = skip > 0.0 ? skip : 12.0;
        const double page = limit > 0.0 ? limit : 600.0;

        const auto* font = suite.fetch(typography::Suite::Face::Regular);
        if (!font) return nullptr;

        const typography::Shaper shaper(arena);
        const Expression evaluator(arena, suite, state.registers());

        const Breaker::Configuration configuration{
            .target = target,
            .leading = leading,
            .tolerance = 2000.0,
            .penalty = 0.0,
            .skip = leading,
            .limit = page
        };

        const Breaker breaker(arena, configuration);

        std::vector<Node*> vertical;
        vertical.reserve(input.size());

        for (std::size_t index = 0uz; index < input.size(); ++index) {
            const auto* item = input[index];
            if (!item) continue;

            if (item->type == syntax::Node::Type::Text || item->type == syntax::Node::Type::Paragraph) {
                memory::Slice<Node*> shaped = shaper.shape(*font, item->value);
                if (shaped.empty()) continue;

                memory::Slice<Node*> lines = breaker.compose(shaped);
                if (lines.empty()) continue;

                if (Node* box = Line::vertical(arena, lines, static_cast<float>(leading))) {
                    vertical.push_back(box);
                }
            } else if (item->type == syntax::Node::Type::Expression) {
                const expression::Unicodes unicodes;
                syntax::Cursor cursor{};
                syntax::Mouth mouth(cursor, state, lexicon, arena);
                mouth.ingest(item->value);

                expression::Parser parser(mouth, unicodes, arena, expression::Style::Display);
                if (const expression::Node* math = parser.parse()) {
                    if (Node* box = evaluator.process(math)) {
                        vertical.push_back(box);
                    }
                }
            }
        }

        if (vertical.empty()) return nullptr;

        memory::Slice<Node*> list = arena.allocate<Node*>(vertical.size());
        std::ranges::copy(vertical, list.begin());

        return Line::vertical(arena, list, 0.0f);
    }

}