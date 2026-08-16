#include "syntax/lexer.hpp"
#include "logger.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace syntax {

    namespace {
        struct Cache {
            const Lexicon* lexicon = nullptr;
            std::array<std::pair<Symbol, std::string_view>, 256> entries{};
        };

        thread_local Cache cache;

        std::pair<Symbol, std::string_view> entry(Lexicon& lexicon, const std::string_view slice) {
            if (slice.size() == 1) {
                const auto index = static_cast<unsigned char>(slice[0]);
                if (cache.lexicon != &lexicon) {
                    cache.lexicon = &lexicon;
                    cache.entries.fill({});
                }
                if (cache.entries[index].first == kInvalidSymbol) {
                    const Symbol symbol = lexicon.intern(slice);
                    cache.entries[index] = {symbol, lexicon.resolve(symbol)};
                }
                return cache.entries[index];
            }
            const Symbol symbol = lexicon.intern(slice);
            return {symbol, lexicon.resolve(symbol)};
        }
    }

    Lexer::Lexer(const std::string_view sources, CatCodes& table, Lexicon& lexicon)
        : sources(sources), table(table), lexicon(lexicon) {
        Logger::fmt(Logger::Type::Lexer, Logger::Level::Informative, "Lexer bound to {} byte buffer", sources.size());
    }

    bool Lexer::empty() const noexcept {
        return offset >= sources.size();
    }

    Token Lexer::advance() {
        const std::size_t size = sources.size();
        while (offset < size) {
            const std::size_t origin = offset;
            const memory::Location position = location;
            const char symbol = sources[offset];
            const CatCodes::Category category = table.get(symbol);

            if (symbol == '\n') {
                offset++;
                location.line++;
                location.column = 1;
                type = Type::Newline;
                const auto [symbol_, value] = entry(lexicon, "\n");
                return Token{symbol_, value, position, CatCodes::Category::Space};
            }

            if (category == CatCodes::Category::Space) {
                offset++;
                location.column++;
                if (type == Type::Skip || type == Type::Newline) continue;
                type = Type::Skip;
                const auto [symbol_, value] = entry(lexicon, " ");
                return Token{symbol_, value, position, CatCodes::Category::Space};
            }

            if (category == CatCodes::Category::Ignore) {
                offset++;
                location.column++;
                continue;
            }

            if (category == CatCodes::Category::Comment) {
                while (offset < size && sources[offset] != '\n') {
                    offset++;
                    location.column++;
                }
                type = Type::Skip;
                continue;
            }

            if (category == CatCodes::Category::Escape) {
                offset++;
                location.column++;

                if (offset < size) {
                    const char next = sources[offset];
                    if (table.get(next) == CatCodes::Category::Letter) {
                        while (offset < size && table.get(sources[offset]) == CatCodes::Category::Letter) {
                            offset++;
                            location.column++;
                        }
                        type = Type::Skip;
                    } else {
                        const std::size_t span = ((static_cast<unsigned char>(next) & 0x80) == 0) ? 1 : std::min(length(next), size - offset);
                        offset += span;
                        location.column++;
                        type = Type::Middle;
                    }
                } else {
                    type = Type::Middle;
                }

                const std::string_view slice = sources.substr(origin, offset - origin);
                const Symbol symbol_ = lexicon.intern(slice);
                const std::string_view value = lexicon.resolve(symbol);
                Logger::fmt(Logger::Type::Lexer, Logger::Level::Debug, "Lexed <Escape> [{}] at {}:{}", slice, position.line, position.column);
                return Token{symbol_, value, position, CatCodes::Category::Escape};
            }

            const std::size_t span = ((static_cast<unsigned char>(symbol) & 0x80) == 0) ? 1 : std::min(length(symbol), size - offset);
            offset += span;
            location.column++;
            type = Type::Middle;

            const std::string_view slice = sources.substr(origin, span);
            const auto [symbol_, value] = entry(lexicon, slice);
            Logger::fmt(Logger::Type::Lexer, Logger::Level::Traceback, "Lexed <{}> [{}] at {}:{}", std::to_underlying(category), slice, position.line, position.column);
            return Token{symbol_, value, position, category};
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