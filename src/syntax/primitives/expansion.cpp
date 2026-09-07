#include "syntax/primitives/expansion.hpp"
#include "logger.hpp"

#include <array>
#include <format>
#include <span>
#include <utility>

namespace syntax::primitives::expansion {

    namespace {
        constexpr std::size_t limit = 1'000'000uz;
    }

    void ingest(Mouth& mouth, semantics::Registers& registers) {
        const Symbol identifier = mouth.lexicon().intern("\\count");

        mouth.bind("\\expandafter", [](Mouth& mouth) {
            const Token delayed = mouth.read();
            const Token evaluated = mouth.expand();
            mouth.inject(std::span{&evaluated, 1});
            mouth.inject(std::span{&delayed, 1});
        });

        mouth.bind("\\noexpand", [](Mouth& mouth) {
            const Token token = mouth.read();
            mouth.inject(std::span{&token, 1});
        });

        mouth.bind("\\expanded", [](Mouth& mouth) {
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::vector<Token> body;
            body.reserve(arguments.size());

            mouth.inject(arguments);
            std::size_t count = 0uz;
            while (true) {
                if (++count > limit) {
                    Logger::log(Logger::Type::Semantics, Logger::Level::Error, "\\expanded exceeded expansion limit");
                    break;
                }
                const Token token = mouth.expand();
                if (token.values.empty()) break;
                body.push_back(token);
            }
            mouth.inject(std::move(body));
        });

        mouth.bind("\\csname", [](Mouth& mouth) {
            std::string content;
            const Symbol terminate = mouth.lexicon().intern("\\endcsname");

            std::size_t count = 0uz;
            while (true) {
                if (++count > limit) {
                    Logger::log(Logger::Type::Semantics, Logger::Level::Error, "\\csname exceeded expansion limit");
                    break;
                }
                const Token token = mouth.expand();
                if (token.symbol == terminate || token.values.empty()) break;
                content += token.values;
            }

            const Symbol reference = mouth.lexicon().intern(content);
            const Token token{reference, CatCodes::Category::Escape, {}, mouth.lexicon().resolve(reference)};
            mouth.inject(std::span{&token, 1});
        });

        mouth.bind("\\string", [](Mouth& mouth) {
            mouth.ingest(std::string(mouth.read().values));
        });

        mouth.bind("\\meaning", [](Mouth& mouth) {
            const Token token = mouth.read();
            std::string content;

            if (mouth.primitive(token.symbol)) content = std::format("primitive {}", token.values);
            else if (mouth.lookup(token.symbol)) content = std::format("macro {}", token.values);
            else content = std::format("undefined {}", token.values);

            mouth.ingest(std::move(content));
        });

        mouth.bind("\\romannumeral", [&registers, identifier](Mouth& mouth) {
            std::int32_t remaining = mouth.integer(registers, identifier).value_or(0);
            static constexpr std::array<std::pair<std::int32_t, std::string_view>, 13> dictionary{{
                {1000, "m"}, {900, "cm"}, {500, "d"}, {400, "cd"},
                {100, "c"},  {90, "xc"},  {50, "l"},   {40, "xl"},
                {10, "x"},   {9, "ix"},   {5, "v"},    {4, "iv"},   {1, "i"}
            }};
            std::string content;
            for (const auto& [base, word] : dictionary) {
                while (remaining >= base) {
                    content += word;
                    remaining -= base;
                }
            }
            mouth.ingest(std::move(content));
        });

        mouth.bind("\\detokenize", [](Mouth& mouth) {
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::string content;
            content.reserve(arguments.size() * 2);
            for (const Token& token : arguments) {
                if (token.category == CatCodes::Category::Escape) content += ' ';
                content += token.values;
            }
            mouth.ingest(std::move(content));
        });

        mouth.bind("\\unexpanded", [](Mouth& mouth) {
            mouth.inject(mouth.argument({}, 0));
        });

        mouth.bind("\\scantokens", [](Mouth& mouth) {
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::string content;
            content.reserve(arguments.size());
            for (const Token& token : arguments) content += token.values;
            mouth.ingest(std::move(content));
        });

        mouth.bind("\\strlen", [](Mouth& mouth) {
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::string content;
            for (const Token& token : arguments) content += token.values;
            mouth.ingest(std::format("{}", content.size()));
        });

        mouth.bind("\\substr", [&registers, identifier](Mouth& mouth) {
            const auto start = mouth.integer(registers, identifier).value_or(0);
            const auto count = mouth.integer(registers, identifier).value_or(0);
            const std::size_t first = start < 0 ? 0uz : static_cast<std::size_t>(start);
            const std::size_t span = count < 0 ? 0uz : static_cast<std::size_t>(count);
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::string content;
            for (const Token& token : arguments) content += token.values;

            if (first < content.size()) mouth.ingest(content.substr(first, span));
        });
    }

}