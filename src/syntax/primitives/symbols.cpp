#include "syntax/primitives/symbols.hpp"
#include <span>

namespace syntax::primitives::symbols {

    void ingest(Mouth& mouth, expression::Unicodes& glyphs, semantics::Registers& registers) {
        const Symbol identifier = mouth.lexicon().intern("\\count");

        mouth.bind("\\mathchardef", [&glyphs, &registers, identifier](Mouth& mouth) {
            const Token token = mouth.read();
            if (token.category != CatCodes::Category::Escape && token.category != CatCodes::Category::Active) return;

            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            const auto value = mouth.integer(registers, identifier);
            if (!value) return;

            Token comma = mouth.read();
            if (comma.values != ",") mouth.inject(std::span{&comma, 1});

            const std::string_view word = mouth.read().values;
            using Category = expression::Unicodes::Category;
            Category category = Category::Ordinary;

            if (word == "operator")         category = Category::Operator;
            else if (word == "binary")      category = Category::Binary;
            else if (word == "relation")    category = Category::Relation;
            else if (word == "opening")     category = Category::Opening;
            else if (word == "closing")     category = Category::Closing;
            else if (word == "punctuation") category = Category::Punctuation;
            else if (word == "inner")       category = Category::Inner;
            else if (word == "accent")      category = Category::Accent;

            glyphs.compose(mouth.lexicon().resolve(token.symbol), static_cast<std::uint32_t>(*value), category);
        });

        mouth.bind("\\undefmathchar", [&glyphs](Mouth& mouth) {
            glyphs.dispose(mouth.lexicon().resolve(mouth.read().symbol));
        });
    }

}