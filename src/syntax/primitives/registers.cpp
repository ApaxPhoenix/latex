#include "syntax/primitives/registers.hpp"

#include <format>
#include <optional>
#include <random>
#include <span>

namespace syntax::primitives::registers {

    void ingest(Mouth& mouth, semantics::Registers& registers) {
        const Symbol count = mouth.lexicon().intern("\\count");
        const Symbol dimension = mouth.lexicon().intern("\\dimen");
        const Symbol glue = mouth.lexicon().intern("\\skip");
        const Symbol token = mouth.lexicon().intern("\\toks");

        static auto allocate = [](Mouth& mouth, const semantics::Registers& registers, const Symbol identifier) -> std::size_t {
            if (const auto value = mouth.integer(registers, identifier)) return static_cast<std::size_t>(*value);
            return 0uz;
        };

        static auto scalar = [](Mouth& mouth, semantics::Registers& registers, const semantics::Registers::Type type, const Symbol identifier, const Symbol size) {
            const bool global = mouth.unglobal();
            const std::size_t index = allocate(mouth, registers, identifier);

            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            const std::int32_t value = (type == semantics::Registers::Type::Count)
                ? mouth.integer(registers, identifier).value_or(0)
                : mouth.dimension(registers, identifier, size).value_or(0);

            registers.assign(type, index, value, global);
        };

        static auto manipulate = [](Mouth& mouth, semantics::Registers& registers, const Symbol count, const Symbol dimension, const Symbol glue, const auto operation) {
            const bool global = mouth.unglobal();
            const Token identifier = mouth.read();
            semantics::Registers::Type type;

            if (identifier.symbol == count) type = semantics::Registers::Type::Count;
            else if (identifier.symbol == dimension) type = semantics::Registers::Type::Dimension;
            else if (identifier.symbol == glue) type = semantics::Registers::Type::Glue;
            else return;

            const std::size_t index = allocate(mouth, registers, count);

            std::size_t offset = 0uz;
            while (mouth.lookahead(offset).category == CatCodes::Category::Space) offset++;

            const auto match = [](const Token& token, const char character) noexcept {
                return token.values.size() == 1uz && (token.values[0] == character || token.values[0] == static_cast<char>(character - ('a' - 'A')));
            };

            if (match(mouth.lookahead(offset), 'b') && match(mouth.lookahead(offset + 1), 'y')) {
                for (std::size_t step = 0uz; step <= offset + 1uz; ++step) mouth.read();
            }

            const std::int32_t value = (type == semantics::Registers::Type::Count)
                ? mouth.integer(registers, count).value_or(0)
                : mouth.dimension(registers, count, dimension).value_or(0);

            const std::int32_t current = registers.fetch(type, index);
            registers.assign(type, index, operation(current, value), global);
        };

        mouth.bind("\\count", [&registers, count, dimension](Mouth& mouth) { scalar(mouth, registers, semantics::Registers::Type::Count, count, dimension); });
        mouth.bind("\\dimen", [&registers, count, dimension](Mouth& mouth) { scalar(mouth, registers, semantics::Registers::Type::Dimension, count, dimension); });
        mouth.bind("\\skip", [&registers, count, dimension](Mouth& mouth) { scalar(mouth, registers, semantics::Registers::Type::Glue, count, dimension); });

        mouth.bind("\\toks", [&registers, count](Mouth& mouth) {
            const bool global = mouth.unglobal();
            const std::size_t index = allocate(mouth, registers, count);

            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            registers.assign(index, mouth.argument({}, 0), global);
        });

        mouth.bind("\\advance", [&registers, count, dimension, glue](Mouth& mouth) {
            manipulate(mouth, registers, count, dimension, glue, [](const std::int32_t left, const std::int32_t right) { return left + right; });
        });
        mouth.bind("\\multiply", [&registers, count, dimension, glue](Mouth& mouth) {
            manipulate(mouth, registers, count, dimension, glue, [](const std::int32_t left, const std::int32_t right) { return left * right; });
        });
        mouth.bind("\\divide", [&registers, count, dimension, glue](Mouth& mouth) {
            manipulate(mouth, registers, count, dimension, glue, [](const std::int32_t left, const std::int32_t right) { return right != 0 ? left / right : left; });
        });
        mouth.bind("\\modulo", [&registers, count, dimension, glue](Mouth& mouth) {
            manipulate(mouth, registers, count, dimension, glue, [](const std::int32_t left, const std::int32_t right) { return right != 0 ? left % right : left; });
        });

        mouth.bind("\\random", [&registers, count](Mouth& mouth) {
            const bool global = mouth.unglobal();
            const std::size_t index = allocate(mouth, registers, count);

            const std::int32_t minimum = mouth.integer(registers, count).value_or(0);
            const std::int32_t maximum = mouth.integer(registers, count).value_or(100);

            static std::mt19937 generator(std::random_device{}());
            std::uniform_int_distribution distribution(minimum, maximum);

            registers.assign(semantics::Registers::Type::Count, index, distribution(generator), global);
        });

        mouth.bind("\\relax", [](Mouth&) {});

        mouth.bind("\\the", [&registers, count, dimension, glue, token](Mouth& mouth) {
            const Token identifier = mouth.read();
            if (identifier.symbol == count) return mouth.ingest(std::format("{}", registers.fetch(semantics::Registers::Type::Count, allocate(mouth, registers, count))));
            if (identifier.symbol == dimension) return mouth.ingest(std::format("{:.2f}pt", registers.fetch(semantics::Registers::Type::Dimension, allocate(mouth, registers, count)) / 65536.0));
            if (identifier.symbol == glue) return mouth.ingest(std::format("{:.2f}pt", registers.fetch(semantics::Registers::Type::Glue, allocate(mouth, registers, count)) / 65536.0));
            if (identifier.symbol == token) return mouth.inject(std::span{registers.get(allocate(mouth, registers, count))});
            mouth.ingest(std::format("{}", registers.get(identifier.symbol)));
        });

        mouth.bind("\\number", [&registers, count](Mouth& mouth) {
            mouth.ingest(std::format("{}", mouth.integer(registers, count).value_or(0)));
        });
    }

}