#include "vm.hpp"
#include "logger.hpp"
#include "syntax/primitives/definitions.hpp"
#include "syntax/primitives/expansion.hpp"
#include "syntax/primitives/input.hpp"
#include "syntax/primitives/registers.hpp"
#include "syntax/primitives/streams.hpp"

#include <fstream>
#include <sstream>

namespace sandbox {

VM::VM(const Policy& policy, std::size_t limit)
    : policy(policy),
      memory(std::make_unique<Allocator>(limit)),
      lexicon(arena),
      gate(lexicon),
      mouth(cursor, state, lexicon, arena)
{
    bind();
}

void VM::bind() {
    syntax::primitives::definitions::ingest(mouth);
    syntax::primitives::expansion::ingest(mouth, registers);
    syntax::primitives::registers::ingest(mouth, registers);
    gate.ingest(mouth, registers);

    if (policy.read || policy.write) {
        syntax::primitives::streams::ingest(mouth, registers, gate);
    }

    if (policy.read) {
        syntax::primitives::input::ingest(mouth);
    }

    if (!policy.shell) {
        mouth.bind("\\pipe", [](syntax::Mouth&) {
            Logger::log(Logger::Type::Semantics, Logger::Level::Warning, "pipe blocked");
        });
    }
}

bool VM::eval(const std::string_view code) {
    try {
        mouth.ingest(code);
        std::size_t count = 0;

        while (mouth.step()) {
            if (++count > policy.tokens) {
                Logger::log(Logger::Type::Semantics, Logger::Level::Error, "token limit reached");
                return false;
            }
        }

        return true;
    } catch (const std::exception& error) {
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Error, "{}", error.what());
        return false;
    }
}

bool VM::run(const std::string_view path) {
    if (!policy.read) {
        Logger::log(Logger::Type::Semantics, Logger::Level::Error, "read access denied");
        return false;
    }

    std::ifstream file{std::string(path)};
    if (!file.is_open()) {
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Error, "file open failed: {}", path);
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return eval(buffer.str());
}

}