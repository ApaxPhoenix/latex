#include "render/primitives/fonts.hpp"

#include "syntax/number.hpp"

#include <span>

namespace render::primitives::fonts {

    const typography::Font* Selection::font() const noexcept {
        return current;
    }

    void Selection::font(const typography::Font* value) noexcept {
        current = value;
    }

    void ingest(
        syntax::Mouth& mouth,
        syntax::semantics::Registers& registers,
        typography::Registry& registry,
        typography::FontConfig& configuration,
        memory::Arena& scratch,
        Selection& selection
    ) {
        const syntax::Symbol identifier = mouth.lexicon().intern("\\count");
        const syntax::Symbol dimension = mouth.lexicon().intern("\\dimen");

        mouth.bind("\\font", [&registers, &registry, &configuration, &scratch, &selection, identifier, dimension](syntax::Mouth& mouth) {
            const syntax::Token target = mouth.read();

            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            const syntax::Token family = mouth.read();

            float size = 12.0f;
            syntax::Token at = mouth.read();
            if (at.values == "at") {
                if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                    size = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
                }
            } else if (!at.empty()) {
                mouth.inject(std::span{&at, 1});
            }

            const auto path = configuration.find(scratch, family.values);
            if (!path) return;

            const typography::Registry::Spec specification{ .family = family.values, .size = size };
            const typography::Font* resolved = registry.get(specification, *path);
            if (!resolved) return;

            mouth.bind(target.symbol, [resolved, &selection](syntax::Mouth&) {
                selection.font(resolved);
            });
        });
    }

}