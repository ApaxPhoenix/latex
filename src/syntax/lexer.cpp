#include "syntax/lexer.hpp"
#include "logger.hpp"

#include <algorithm>

namespace syntax {

    Lexer::Lexer(const std::string_view source, CatCodes& table, Lexicon& lexicon)
        : source(source), table(table), lexicon(lexicon) {
        Logger::fmt(Logger::Type::Lexer, Logger::Level::Informative,
                    "Lexer initialized with source size {} bytes", source.size());
    }

    bool Lexer::empty() const noexcept {
        return offset >= source.size();
    }

    Token Lexer::advance() {
        while (offset < source.size()) {
            const std::size_t origin = offset;
            const memory::Location position = location;
            const char symbol = source[offset];
            const CatCodes::Type category = table.get(symbol);

            if (symbol == '\n') {
                offset++;
                location.line++;
                location.column = 1;
                type = Type::Newline;
                constexpr std::string_view value = "\n";
                const Symbol sym = lexicon.intern(value);
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback,
                            "Lexed Newline token at line {} col {}", position.line, position.column);
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Type::Space};
            }

            if (category == CatCodes::Type::Space) {
                offset++;
                location.column++;
                if (type == Type::Skip || type == Type::Newline) {
                    continue;
                }
                type = Type::Skip;
                constexpr std::string_view value = " ";
                const Symbol sym = lexicon.intern(value);
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback,
                            "Lexed Space token at line {} col {}", position.line, position.column);
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Type::Space};
            }

            if (category == CatCodes::Type::Ignore) {
                offset++;
                location.column++;
                continue;
            }

            if (category == CatCodes::Type::Comment) {
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback,
                            "Skipping comment block starting at line {} col {}", position.line, position.column);
                while (offset < source.size() && source[offset] != '\n') {
                    offset++;
                    location.column++;
                }
                type = Type::Skip;
                continue;
            }

            if (category == CatCodes::Type::Escape) {
                offset++;
                location.column++;

                if (offset < source.size()) {
                    if (const char next = source[offset]; table.get(next) == CatCodes::Type::Letter) {
                        while (offset < source.size() && table.get(source[offset]) == CatCodes::Type::Letter) {
                            const std::size_t span = std::min(
                                length(source[offset]),
                                source.size() - offset
                            );
                            offset += span;
                            location.column++;
                        }
                        type = Type::Skip;
                    } else {
                        const std::size_t span = std::min(
                            length(next),
                            source.size() - offset
                        );
                        offset += span;
                        location.column++;
                        type = Type::Middle;
                    }
                } else {
                    type = Type::Middle;
                }

                const std::string_view slice = source.substr(origin, offset - origin);
                const Symbol sym = lexicon.intern(slice);
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Debug,
                            "Lexed Control Sequence Escape token '{}' at line {} col {}",
                            slice, position.line, position.column);
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Type::Escape};
            }

            const std::size_t span = std::min(
                length(symbol),
                source.size() - offset
            );
            offset += span;
            location.column++;
            type = Type::Middle;

            const std::string_view slice = source.substr(origin, span);
            const Symbol sym = lexicon.intern(slice);
            Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback,
                        "Lexed Character token '{}' (catcode={}) at line {} col {}",
                        slice, static_cast<std::uint32_t>(category), position.line, position.column);
            return Token{sym, lexicon.resolve(sym), position, category};
        }

        Logger::log(Logger::Type::Lexer, Logger::Level::Debug, "Lexer reached end of source stream");
        return {};
    }

    void Lexer::reset() noexcept {
        offset = 0;
        location = {1, 1};
        type = Type::Newline;
        Logger::log(Logger::Type::Lexer, Logger::Level::Debug, "Lexer state reset to default position");
    }

}