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
        buffer.reserve(256);

        using Location = decltype(mouth_.expand().location);
        Location position{};

        auto flush = [&] {
            if (!buffer.empty()) {
                nodes.push_back(arena_.compose<Node>(
                    Node::Type::Text,
                    arena_.copy(buffer),
                    position,
                    memory::Slice<Node*>{}
                ));
                buffer.clear();
            }
        };

        while (true) {
            const auto [symbol, category, location, values] = mouth_.expand();
            if (values.empty()) {
                flush();
                Logger::log(Logger::Type::Parser, Logger::Level::Debug, "Parser reached end of expansion stream");
                break;
            }

            if (symbol < handlers.size() && handlers[symbol]) {
                flush();
                Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                            "Dispatching custom node handler for text {} ('{}')", symbol, values);
                if (Node* node = handlers[symbol](*this)) {
                    nodes.push_back(node);
                }
                continue;
            }

            if (symbol == paragraph) {
                flush();
                Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                            "Constructed Paragraph node at line {} column {}", location.line, location.column);
                nodes.push_back(arena_.compose<Node>(Node::Type::Paragraph, values, location, memory::Slice<Node*>{}));
                continue;
            }

            if (category == CatCodes::Category::Escape && (symbol >= handlers.size() || !handlers[symbol])) {
                flush();
                const std::string message = "Undefined macro or unhandled command primitive: " + std::string(values);

                Logger::fmt(Logger::Type::Parser, Logger::Level::Error,
                            "Parsing error at line {} column {}: {}", location.line, location.column, message);

                this->tracebacks_.emplace_back(
                    Traceback::Type::Macro,
                    location,
                    message
                );

                while (true) {
                    const Token token = mouth_.expand();
                    if (token.values.empty()) {
                        Logger::log(Logger::Type::Parser, Logger::Level::Debug, "Recovery reached stream end");
                        break;
                    }
                    if (token.symbol == paragraph) {
                        Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                                    "Recovered at paragraph synchronization boundary at line {} column {}", token.location.line, token.location.column);
                        nodes.push_back(arena_.compose<Node>(Node::Type::Paragraph, token.values, token.location, memory::Slice<Node*>{}));
                        break;
                    }
                }
                continue;
            }

            if (buffer.empty()) {
                position = location;
            }
            buffer += values;
        }

        if (!this->tracebacks_.empty()) {
            Logger::fmt(Logger::Type::Parser, Logger::Level::Warning,
                        "Parse pass completed with {} traceback error(s) recorded", this->tracebacks_.size());

            for (const auto& traceback : this->tracebacks_) {
                const std::string message = traceback.format();
                Logger::log(Logger::Type::Parser, Logger::Level::Error, message);
                std::cerr << message << "\n";
            }
        } else {
            Logger::log(Logger::Type::Parser, Logger::Level::Informative, "Parse pass completed cleanly with zero errors");
        }

        memory::Slice<Node*> slice = arena_.allocate<Node*>(nodes.size());
        if (!nodes.empty()) {
            std::ranges::copy(nodes, slice.begin());
        }

        Logger::fmt(Logger::Type::Parser, Logger::Level::Informative,
                    "Allocated AST slice containing {} node pointers in arena", slice.size());

        return slice;
    }

    void Parser::bind(const std::string_view name, Handler handler) {
        this->bind(mouth_.lexicon().intern(name), std::move(handler));
    }

    void Parser::bind(const Symbol symbol, Handler handler) {
        const auto size = static_cast<std::size_t>(symbol) + 1;
        if (symbol >= handlers.size()) {
            handlers.resize(std::max<std::size_t>(size, handlers.size() * 2));
        }
        Logger::fmt(Logger::Type::Parser, Logger::Level::Debug,
                    "Bound custom AST node handler for text {}", symbol);
        handlers[symbol] = std::move(handler);
    }

}