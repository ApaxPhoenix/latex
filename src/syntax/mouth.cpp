#include "syntax/mouth.hpp"
#include "syntax/lexer.hpp"
#include "syntax/number.hpp"
#include "memory/location.hpp"
#include "logger.hpp"

#include <algorithm>
#include <format>
#include <utility>

namespace syntax {

    Mouth::Mouth(Cursor cursor, semantics::Union& state, Lexicon& lexicon, memory::Arena& arena)
        : cursor(std::move(cursor)), union_(state), lexicon_(lexicon), arena(arena) {}

    void Mouth::push() {
        this->marks.push_back(this->records.size());
        this->union_.push();
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "State push (depth={})", this->marks.size());
    }

    void Mouth::pop() {
        if (this->marks.empty()) {
            const memory::Location location = this->cursor.empty() ? memory::Location{} : this->cursor.lookahead(0).location;
            const std::string message = "Dangling scope pop boundary constraint violation";
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Error, "Scope pop failure at {}:{}", location.line, location.column);
            this->tracebacks_.emplace_back(Traceback::Type::Scope, location, message);
            return;
        }

        this->union_.pop();

        const std::size_t mark = this->marks.back();
        this->marks.pop_back();

        for (std::size_t count = this->records.size(); count > mark; --count) {
            auto [symbol_, macros_, active_] = std::move(this->records.back());
            this->records.pop_back();
            if (symbol_ < this->macros.size()) {
                this->macros[symbol_] = std::move(macros_);
                this->macros[symbol_].active = active_;
            }
        }
    }

    void Mouth::globalize() noexcept {
        this->global = true;
    }

    Token Mouth::read() noexcept {
        if (this->cursor.empty()) return {};
        return this->cursor.advance();
    }

    Token Mouth::expand() {
        while (!this->cursor.empty()) {
            if (!this->step()) return this->cursor.advance();
        }
        return {};
    }

    bool Mouth::step() {
        if (this->cursor.empty()) return false;

        const Token token = this->cursor.lookahead(0);
        if (token.symbol == kInvalidSymbol) return false;

        if (this->suppress) {
            this->suppress = false;
            return false;
        }

        if (token.symbol < this->handler.size() && this->handler[token.symbol]) {
            this->cursor.advance();
            this->handler[token.symbol](*this);
            return true;
        }

        if (token.symbol < this->macros.size() && this->macros[token.symbol].active) {
            this->cursor.advance();

            if (this->depth >= this->limit) {
                const std::string message = std::format("Macro recursion threshold {} exceeded", this->limit);
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Error, "Recursion limit hit at {}:{}", token.location.line, token.location.column);
                this->tracebacks_.emplace_back(Traceback::Type::Recursion, token.location, message);
                return false;
            }
            this->depth++;

            const auto& macro = this->macros[token.symbol];
            std::vector<std::vector<Token>> arguments;
            arguments.reserve(macro.parameters.size());
            for (const auto& parameter : macro.parameters) {
                arguments.push_back(this->argument(parameter));
            }

            std::vector<Token> output;
            const auto& body = macro.body;
            std::size_t total = body.size();
            for (const auto& item : arguments) {
                total += item.size();
            }
            output.reserve(total);

            for (std::size_t index = 0; index < body.size(); ++index) {
                if (body[index].category == CatCodes::Category::Parameter && index + 1 < body.size()) {
                    if (const auto& values = body[index + 1].values; !values.empty() && values[0] >= '1' && values[0] <= '9') {
                        if (const auto slot = static_cast<std::size_t>(values[0] - '1'); slot < arguments.size()) {
                            output.insert(output.end(), arguments[slot].begin(), arguments[slot].end());
                        }
                        index++;
                        continue;
                    }
                }
                output.push_back(body[index]);
            }

            this->cursor.inject(output);
            this->depth--;
            return true;
        }

        return false;
    }

    std::vector<Token> Mouth::argument(const Parameter& parameter) {
        std::vector<Token> result;
        if (this->cursor.empty()) return result;

        if (parameter.optional) {
            const Token lead = this->cursor.lookahead(0);
            if (lead.category == CatCodes::Category::Other && lead.values == "[") {
                this->cursor.advance();
                std::size_t scope = 1;
                while (!this->cursor.empty() && scope > 0) {
                    Token token = this->cursor.advance();
                    if (token.values == "[") scope++;
                    else if (token.values == "]") scope--;
                    if (scope > 0) result.push_back(token);
                }
            } else {
                result = parameter.fallbacks;
            }
        } else if (!parameter.delimiters.empty()) {
            std::size_t scope = 0;
            bool matched = false;
            const std::size_t count = parameter.delimiters.size();

            while (!this->cursor.empty()) {
                if (scope == 0) {
                    bool found = true;
                    for (std::size_t index = 0; index < count; ++index) {
                        if (this->cursor.lookahead(index).symbol != parameter.delimiters[index].symbol) {
                            found = false;
                            break;
                        }
                    }

                    if (found) {
                        matched = true;
                        for (std::size_t index = 0; index < count; ++index) {
                            this->cursor.advance();
                        }
                        break;
                    }
                }

                Token token = this->cursor.advance();
                if (token.category == CatCodes::Category::Group) {
                    if (token.values == "{") scope++;
                    else if (token.values == "}" && scope > 0) scope--;
                }
                result.push_back(token);
            }

            if (!matched) {
                const memory::Location location = this->cursor.empty() ? memory::Location{} : this->cursor.lookahead(0).location;
                const std::string message = "Delimited macro expansion missing parameter bounding tokens";
                Logger::log(Logger::Type::Mouth, Logger::Level::Warning, message);
                this->tracebacks_.emplace_back(Traceback::Type::Delimiter, location, message);
            }
        } else {
            while (!this->cursor.empty() && this->cursor.lookahead(0).category == CatCodes::Category::Space) {
                this->cursor.advance();
            }

            if (const Token token = this->cursor.advance(); token.category == CatCodes::Category::Group && token.values == "{") {
                std::size_t scope = 1;
                while (!this->cursor.empty() && scope > 0) {
                    Token element = this->cursor.advance();
                    if (element.category == CatCodes::Category::Group) {
                        if (element.values == "{") scope++;
                        else if (element.values == "}") scope--;
                    }
                    if (scope > 0) result.push_back(element);
                }
                if (scope > 0) {
                    const std::string message = "Macro argument block truncation missing trailing brace";
                    Logger::log(Logger::Type::Mouth, Logger::Level::Warning, message);
                    this->tracebacks_.emplace_back(Traceback::Type::Group, token.location, message);
                }
            } else {
                if (token.symbol != kInvalidSymbol || !token.values.empty()) {
                    result.push_back(token);
                }
            }
        }
        return result;
    }

    void Mouth::inject(const std::span<const Token> tokens) {
        this->cursor.inject(tokens);
    }

    void Mouth::ingest(const std::string_view source) {
        Lexer lexer(source, this->union_.catcodes(), this->lexicon_);
        std::vector<Token> tokens;
        tokens.reserve(source.size() / 2);

        while (!lexer.empty()) {
            if (Token token = lexer.advance(); !token.values.empty()) {
                tokens.push_back(token);
            }
        }
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "Lexed buffer mapping block elements size {}", tokens.size());
        this->cursor.inject(tokens);
    }

    void Mouth::bind(const std::string_view name, Handler handler) {
        this->bind(this->lexicon_.intern(name), std::move(handler));
    }

    void Mouth::bind(const Symbol symbol, Handler handler) {
        const auto needed = static_cast<std::size_t>(symbol) + 1;
        if (symbol >= this->handler.size()) {
            this->handler.resize(std::max<std::size_t>(needed, this->handler.size() * 2));
        }
        this->handler[symbol] = std::move(handler);
    }

    void Mouth::define(const std::string_view name, Macro macro) {
        this->define(this->lexicon_.intern(name), std::move(macro));
    }

    void Mouth::define(const Symbol symbol, Macro macro) {
        const auto slot = static_cast<std::size_t>(symbol);

        if (!this->global && !this->marks.empty()) {
            const bool active = (slot < this->macros.size() && this->macros[slot].active);
            const Macro previous = active ? this->macros[slot] : Macro{};
            this->records.push_back(Record{symbol, previous, active});
        }

        if (slot >= this->macros.size()) {
            this->macros.resize(std::max<std::size_t>(slot + 1, this->macros.size() * 2));
        }
        macro.active = true;
        this->macros[slot] = std::move(macro);
        this->global = false;
    }

    void Mouth::undefine(const std::string_view name) {
        this->undefine(this->lexicon_.intern(name));
    }

    void Mouth::undefine(const Symbol symbol) {
        if (const auto slot = static_cast<std::size_t>(symbol); slot < this->macros.size() && this->macros[slot].active) {
            if (!this->global && !this->marks.empty()) {
                this->records.push_back(Record{symbol, this->macros[slot], true});
            }
            this->macros[slot].active = false;
        }
        this->global = false;
    }

}