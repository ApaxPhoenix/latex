#include "syntax/primitives/streams.hpp"
#include "syntax/primitives/board.hpp"
#include "syntax/semantics/union.hpp"
#include "syntax/lexer.hpp"
#include "logger.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

namespace syntax::primitives::streams {

    namespace {

        bool confined(const std::string_view path) {
            namespace filesystem = std::filesystem;

            std::error_code error;
            const filesystem::path root = filesystem::current_path(error);
            if (error) return false;

            const filesystem::path target = filesystem::weakly_canonical(root / filesystem::path(path), error);
            if (error) return false;

            const auto inside = std::mismatch(root.begin(), root.end(), target.begin(), target.end()).first == root.end();

            if (!inside) {
                Logger::fmt(Logger::Type::Semantics, Logger::Level::Warning,
                            "Blocked filesystem access outside sandbox root: {}", path);
            }
            return inside;
        }

    }

    void ingest(Mouth& mouth, semantics::Registers& registers, const conditionals::Gate& gate) {
        const Symbol identifier = mouth.lexicon().intern("\\count");

        static auto allocate = [](Mouth& mouth, const semantics::Registers& registers, const Symbol identifier) -> std::size_t {
            const auto value = mouth.integer(registers, identifier);
            if (!value || *value < 0) return 0uz;
            return static_cast<std::size_t>(*value);
        };

        mouth.bind("\\openin", [&registers, identifier](Mouth& mouth) {
            const std::size_t index = allocate(mouth, registers, identifier);
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            const Token filename = mouth.read();
            if (!confined(filename.values)) return;
            board().reader.open(index, filename.values);
        });

        mouth.bind("\\closein", [&registers, identifier](Mouth& mouth) {
            board().reader.close(allocate(mouth, registers, identifier));
        });

        mouth.bind("\\read", [&registers, identifier](Mouth& mouth) {
            const std::size_t index = allocate(mouth, registers, identifier);
            Token keyword = mouth.read();
            if (keyword.values != "to") mouth.inject(std::span{&keyword, 1});
            const Token token = mouth.read();

            if (token.category != CatCodes::Category::Escape && token.category != CatCodes::Category::Active) return;

            std::string content;
            board().reader.read(index, content);

            Lexer lexer(content, mouth.state().catcodes(), mouth.lexicon());
            std::vector<Token> body;
            while (!lexer.empty()) {
                if (Token item = lexer.advance(); !item.values.empty()) body.push_back(item);
            }

            Mouth::Macro macro;
            macro.body = std::move(body);
            mouth.define(token.symbol, std::move(macro));
        });

        mouth.bind("\\ifeof", [&registers, identifier, &gate](Mouth& mouth) {
            gate.path(mouth, board().reader.finished(allocate(mouth, registers, identifier)));
        });

        mouth.bind("\\openout", [&registers, identifier](Mouth& mouth) {
            const std::size_t index = allocate(mouth, registers, identifier);
            Token equals = mouth.read();
            if (equals.values != "=") mouth.inject(std::span{&equals, 1});

            const Token filename = mouth.read();
            if (!confined(filename.values)) return;
            board().writer.open(index, filename.values);
        });

        mouth.bind("\\closeout", [&registers, identifier](Mouth& mouth) {
            board().writer.close(allocate(mouth, registers, identifier));
        });

        mouth.bind("\\write", [&registers, identifier](Mouth& mouth) {
            board().writer.write(allocate(mouth, registers, identifier), mouth.read().values);
        });

        mouth.bind("\\pipe", [](Mouth& mouth) {
            const std::vector<Token> arguments = mouth.argument({}, 0);
            std::string command;
            for (const Token& token : arguments) command += token.values;

            std::array<char, 128> buffer{};
            std::string output;
            #if defined(_WIN32)
                        const std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
            #else
                        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
            #endif
            if (pipe) {
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) output += buffer.data();
            }
            mouth.ingest(output);
        });
    }

}