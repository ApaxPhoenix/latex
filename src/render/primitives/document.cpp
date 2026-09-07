#include "render/primitives/document.hpp"

#include "syntax/number.hpp"

#include <span>
#include <string>
#include <vector>

namespace render::primitives::document {

    void ingest(syntax::Mouth& mouth, layout::Document& document, syntax::semantics::Registers& registers) {
        const syntax::Symbol identifier = mouth.lexicon().intern("\\count");
        const syntax::Symbol dimension = mouth.lexicon().intern("\\dimen");

        mouth.bind("\\pagewidth", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().width = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\pageheight", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().height = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\leftmargin", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().left = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\rightmargin", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().right = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\topmargin", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().top = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\bottommargin", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().bottom = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\leading", [&document, &registers, identifier, dimension](syntax::Mouth& mouth) {
            syntax::Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});
            if (const auto value = mouth.dimension(registers, identifier, dimension)) {
                document.configuration().leading = static_cast<float>(*value) / static_cast<float>(syntax::Number::scale);
            }
        });

        mouth.bind("\\documentclass", [&document](syntax::Mouth& mouth) {
            const std::vector<syntax::Token> tokens = mouth.argument({}, 0);
            std::string name;
            for (const syntax::Token& token : tokens) name += token.values;

            layout::Document::Configuration configuration = document.configuration();

            if (name == "article" || name == "letter") {
                configuration.width = 612.0f;
                configuration.height = 792.0f;
                configuration.left = 72.0f;
                configuration.right = 72.0f;
                configuration.top = 72.0f;
                configuration.bottom = 72.0f;
            } else if (name == "book" || name == "report") {
                configuration.width = 612.0f;
                configuration.height = 792.0f;
                configuration.left = 90.0f;
                configuration.right = 54.0f;
                configuration.top = 72.0f;
                configuration.bottom = 72.0f;
            } else if (name == "a4paper") {
                configuration.width = 595.0f;
                configuration.height = 842.0f;
            }

            document.configuration() = configuration;
        });
    }

}