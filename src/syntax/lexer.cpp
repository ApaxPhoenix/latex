#include "syntax/lexer.hpp"
#include "logger.hpp"

#include <algorithm>
#include <utility>

namespace syntax {

    Lexer::Lexer(const std::string_view sources, CatCodes& table, Lexicon& lexicon)
        : sources(sources), table(table), lexicon(lexicon) {
        Logger::fmt(Logger::Type::Lexer, Logger::Level::Informative, "Lexer bound to {} byte buffer", sources.size());
    }

    bool Lexer::empty() const noexcept {
        return offset >= sources.size();
    }

    Token Lexer::advance() {
        while (offset < sources.size()) {
            const std::size_t origin = offset;
            const memory::Location position = location;
            const char symbol = sources[offset];
            const CatCodes::Category category = table.get(symbol);

            if (symbol == '\n') {
                offset++;
                location.line++;
                location.column = 1;
                type = Type::Newline;
                const Symbol sym = lexicon.intern("\n");
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Category::Space};
            }

            if (category == CatCodes::Category::Space) {
                offset++;
                location.column++;
                if (type == Type::Skip || type == Type::Newline) continue;
                type = Type::Skip;
                const Symbol sym = lexicon.intern(" ");
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Category::Space};
            }

            if (category == CatCodes::Category::Ignore) {
                offset++;
                location.column++;
                continue;
            }

            if (category == CatCodes::Category::Comment) {
                while (offset < sources.size() && sources[offset] != '\n') {
                    offset++;
                    location.column++;
                }
                type = Type::Skip;
                continue;
            }

            if (category == CatCodes::Category::Escape) {
                offset++;
                location.column++;

                if (offset < sources.size()) {
                    if (const char next = sources[offset]; table.get(next) == CatCodes::Category::Letter) {
                        while (offset < sources.size() && table.get(sources[offset]) == CatCodes::Category::Letter) {
                            const std::size_t span = std::min(length(sources[offset]), sources.size() - offset);
                            offset += span;
                            location.column++;
                        }
                        type = Type::Skip;
                    } else {
                        const std::size_t span = std::min(length(next), sources.size() - offset);
                        offset += span;
                        location.column++;
                        type = Type::Middle;
                    }
                } else {
                    type = Type::Middle;
                }

                const std::string_view slice = sources.substr(origin, offset - origin);
                const Symbol sym = lexicon.intern(slice);
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Debug, "Lexed <Escape> [{}] at {}:{}", slice, position.line, position.column);
                return Token{sym, lexicon.resolve(sym), position, CatCodes::Category::Escape};
            }

            const std::size_t span = std::min(length(symbol), sources.size() - offset);
            offset += span;
            location.column++;
            type = Type::Middle;

            const std::string_view slice = sources.substr(origin, span);
            const Symbol sym = lexicon.intern(slice);
            Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback, "Lexed <{}> [{}] at {}:{}", std::to_underlying(category), slice, position.line, position.column);
            return Token{sym, lexicon.resolve(sym), position, category};
        }

        return {};
    }

    void Lexer::reset() noexcept {
        offset = 0;
        location = {1, 1};
        type = Type::Newline;
        Logger::log(Logger::Type::Lexer, Logger::Level::Debug, "Lexer state reset");
    }

}