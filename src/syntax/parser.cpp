#include "syntax/parser.hpp"
#include "logger.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace syntax {

    Parser::Parser(Mouth& mouth, memory::Arena& arena)
        : mouth_(mouth), arena_(arena) {
        Logger::log(Logger::Type::Parser, Logger::Level::Informative, "Parser subsystem initialized");
    }

    Token Parser::step() const {
        return mouth_.expand();
    }

    memory::Slice<Node*> Parser::parse() {
        Logger::log(Logger::Type::Parser, Logger::Level::Informative, "Starting AST syntax parsing pass...");

        std::vector<Node*> nodes;
        nodes.reserve(1024);

        const Symbol paragraph = mouth_.lexicon().intern("\\par");
        std::string buffer;

        using Location = decltype(mouth_.expand().location);
        Location position{};

        auto flush = [&]() {
            if (!buffer.empty()) {
                nodes.push_back(arena_.compose<Node>(
                    Node::Type::Text,
                    buffer,
                    position,
                    memory::Slice<Node*>{}
                ));
                buffer.clear();
            }
        };

        while (true) {
            auto [symbol, value, current, type] = mouth_.expand();
            if (value.empty()) {
                flush();
                Logger::log(Logger::Type::Parser, Logger::Level::Debug, "Parser reached end of expansion stream");
                break;
            }

            if (symbol < handlers_.size() && handlers_[symbol]) {
                flush();
                Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                            "Dispatching custom node handler for text {} ('{}')", symbol, value);
                if (Node* node = handlers_[symbol](*this)) {
                    nodes.push_back(node);
                }
                continue;
            }

            if (symbol == paragraph) {
                flush();
                Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                            "Constructed Paragraph node at line {} col {}", current.line, current.column);
                nodes.push_back(arena_.compose<Node>(Node::Type::Paragraph, value, current, memory::Slice<Node*>{}));
                continue;
            }

            if (type == CatCodes::Type::Escape && symbol >= handlers_.size()) {
                flush();
                const std::string message = "Undefined macro or unhandled command primitive: " + std::string(value);

                Logger::fmt(Logger::Type::Parser, Logger::Level::Error,
                            "Parsing error at line {} col {}: {}", current.line, current.column, message);

                mouth_.tracebacks().emplace_back(
                    Traceback::Type::Macro,
                    current,
                    message
                );
                continue;
            }

            if (buffer.empty()) {
                position = current;
            }
            buffer += value;
        }

        if (!mouth_.tracebacks().empty()) {
            Logger::fmt(Logger::Type::Parser, Logger::Level::Warning,
                        "Parse pass completed with {} traceback error(s) recorded", mouth_.tracebacks().size());

            std::cerr << "[Parser Traceback Log]\n";
            for (const auto& traceback : mouth_.tracebacks()) {
                const std::string formatted = traceback.format();
                Logger::log(Logger::Type::Parser, Logger::Level::Error, formatted);
                std::cerr << formatted << "\n";
            }
        } else {
            Logger::log(Logger::Type::Parser, Logger::Level::Informative, "Parse pass completed cleanly with zero errors");
        }

        memory::Slice<Node*> slice = arena_.allocate<Node*>(nodes.size());
        std::ranges::copy(nodes, slice.begin());

        Logger::fmt(Logger::Type::Parser, Logger::Level::Informative,
                    "Allocated AST slice containing {} node pointers in arena", slice.size());

        return slice;
    }

    void Parser::bind(const std::string_view name, Handler handler) {
        this->bind(mouth_.lexicon().intern(name), std::move(handler));
    }

    void Parser::bind(const Symbol symbol, Handler handler) {
        const auto size = static_cast<std::size_t>(symbol) + 1;
        if (symbol >= handlers_.size()) {
            handlers_.resize(std::max<std::size_t>(size, handlers_.size() * 2));
        }
        Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                    "Bound custom AST node handler for text {}", symbol);
        handlers_[symbol] = std::move(handler);
    }

}