#include "syntax/primitives/conditionals.hpp"

namespace syntax::primitives::conditionals {

    Gate::Gate(Lexicon& lexicon) noexcept {
        finish = lexicon.intern("\\fi");
        alternative = lexicon.intern("\\else");
        branch = lexicon.intern("\\or");
        identifier = lexicon.intern("\\count");
        dimension = lexicon.intern("\\dimen");
        terminate = lexicon.intern("\\endcsname");
    }

    bool Gate::compare(const Mouth::Macro& first, const Mouth::Macro& second) noexcept {
        if (first.parameters.size() != second.parameters.size()) return false;
        if (first.body.size() != second.body.size()) return false;
        for (std::size_t index = 0uz; index < first.body.size(); ++index) {
            if (first.body[index].symbol != second.body[index].symbol) return false;
            if (first.body[index].values != second.body[index].values) return false;
        }
        return true;
    }

    void Gate::skip(Mouth& mouth) const noexcept {
        std::size_t depth = 0uz;
        while (true) {
            const Token token = mouth.read();
            if (token.values.empty()) return;

            if (token.category == CatCodes::Category::Escape && token.values.starts_with("\\if")) {
                depth++;
                continue;
            }
            if (token.symbol == finish) {
                if (depth == 0uz) return;
                depth--;
                continue;
            }
            if (depth == 0uz && token.symbol == alternative) return;
        }
    }

    void Gate::drop(Mouth& mouth) const noexcept {
        std::size_t depth = 0uz;
        while (true) {
            const Token token = mouth.read();
            if (token.values.empty()) return;

            if (token.category == CatCodes::Category::Escape && token.values.starts_with("\\if")) {
                depth++;
                continue;
            }
            if (token.symbol == finish) {
                if (depth == 0uz) return;
                depth--;
            }
        }
    }

    void Gate::path(Mouth& mouth, bool condition) const noexcept {
        if (inverted) {
            condition = !condition;
            inverted = false;
        }
        if (!condition) skip(mouth);
    }

    void Gate::ingest(Mouth& mouth, semantics::Registers& registers) const {
        mouth.bind("\\iftrue", [this](Mouth& mouth) { path(mouth, true); });
        mouth.bind("\\iffalse", [this](Mouth& mouth) { path(mouth, false); });
        mouth.bind("\\unless", [this](Mouth&) { inverted = true; });

        mouth.bind("\\ifx", [this](Mouth& mouth) {
            const Token first = mouth.read();
            const Token second = mouth.read();
            const auto primary = mouth.lookup(first.symbol);
            const auto secondary = mouth.lookup(second.symbol);
            bool condition = false;

            if (primary && secondary) condition = compare(*primary, *secondary);
            else if (primary || secondary) condition = false;
            else {
                const bool start = static_cast<bool>(mouth.primitive(first.symbol));
                const bool end = static_cast<bool>(mouth.primitive(second.symbol));
                condition = start || end ? start && end && first.symbol == second.symbol : true;
            }
            path(mouth, condition);
        });

        mouth.bind("\\ifcat", [this](Mouth& mouth) {
            path(mouth, mouth.read().category == mouth.read().category);
        });

        mouth.bind("\\if", [this](Mouth& mouth) {
            const Token first = mouth.expand();
            const Token second = mouth.expand();
            path(mouth, first.symbol == second.symbol || first.values == second.values);
        });

        mouth.bind("\\ifempty", [this](Mouth& mouth) {
            path(mouth, mouth.argument({}, 0).empty());
        });

        mouth.bind("\\ifdefined", [this](Mouth& mouth) {
            const Token token = mouth.read();
            path(mouth, mouth.lookup(token.symbol).has_value() || static_cast<bool>(mouth.primitive(token.symbol)));
        });

        mouth.bind("\\ifcsname", [this](Mouth& mouth) {
            std::string content;
            while (true) {
                const Token token = mouth.expand();
                if (token.symbol == terminate || token.values.empty()) break;
                content += token.values;
            }
            path(mouth, mouth.lookup(mouth.lexicon().intern(content)).has_value());
        });

        mouth.bind("\\ifnum", [this, &registers](Mouth& mouth) {
            const auto first = mouth.integer(registers, identifier);
            const Token operator_ = mouth.read();
            const auto second = mouth.integer(registers, identifier);
            bool condition = false;

            if (first && second) {
                if (operator_.values == "<") condition = *first < *second;
                else if (operator_.values == ">") condition = *first > *second;
                else if (operator_.values == "=") condition = *first == *second;
            }
            path(mouth, condition);
        });

        mouth.bind("\\ifdim", [this, &registers](Mouth& mouth) {
            const auto first = mouth.dimension(registers, identifier, dimension);
            const Token operator_ = mouth.read();
            const auto second = mouth.dimension(registers, identifier, dimension);
            bool condition = false;

            if (first && second) {
                if (operator_.values == "<") condition = *first < *second;
                else if (operator_.values == ">") condition = *first > *second;
                else if (operator_.values == "=") condition = *first == *second;
            }
            path(mouth, condition);
        });

        mouth.bind("\\ifodd", [this, &registers](Mouth& mouth) {
            const auto value = mouth.integer(registers, identifier);
            path(mouth, value.has_value() && (*value % 2) != 0);
        });

        mouth.bind("\\ifcase", [this, &registers](Mouth& mouth) {
            if (inverted) inverted = false;
            std::int32_t remaining = mouth.integer(registers, identifier).value_or(0);
            if (remaining < 0) {
                skip(mouth);
                return;
            }

            std::size_t depth = 0uz;
            while (remaining > 0) {
                const Token token = mouth.read();
                if (token.values.empty()) return;

                if (token.category == CatCodes::Category::Escape && token.values.starts_with("\\if")) {
                    depth++;
                    continue;
                }
                if (token.symbol == finish) {
                    if (depth == 0uz) return;
                    depth--;
                    continue;
                }
                if (depth == 0uz && token.symbol == alternative) return;
                if (depth == 0uz && token.symbol == branch) remaining--;
            }
        });

        mouth.bind("\\else", [this](Mouth& mouth) { drop(mouth); });
        mouth.bind("\\or", [this](Mouth& mouth) { drop(mouth); });
        mouth.bind("\\fi", [](Mouth&) {});
    }

}