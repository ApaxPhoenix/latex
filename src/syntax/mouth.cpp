#include "syntax/mouth.hpp"
#include "syntax/lexer.hpp"
#include "syntax/number.hpp"
#include "semantics/union.hpp"
#include "memory/location.hpp"
#include "logger.hpp"

#include <algorithm>
#include <format>
#include <utility>

namespace syntax {

    static std::optional<std::size_t> probe(const Cursor& cursor, const std::vector<Token>& delimiters) noexcept {
        std::size_t offset = 0;

        for (const Token& want : delimiters) {
            if (want.category == CatCodes::Category::Space) {
                while (cursor.lookahead(offset).category == CatCodes::Category::Space) {
                    offset++;
                }
                continue;
            }

            if (const Token got = cursor.lookahead(offset); got.values.empty() || got.symbol != want.symbol) {
                return std::nullopt;
            }
            offset++;
        }

        return offset;
    }

    Mouth::Mouth(Cursor input_cursor, semantics::Union& state, Lexicon& lexicon, memory::Arena& arena)
        : cursor(std::move(input_cursor)), state_(state), lexicon_(lexicon), arena_(arena) {
        this->paragraph = this->lexicon_.intern("\\par");
    }

    void Mouth::push() {
        this->marks.push_back(this->records.size());
        this->deferred.emplace_back();
        this->state_.push();
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug, "State push (depth={})", this->marks.size());
    }

    void Mouth::pop() {
        if (this->marks.empty()) {
            const memory::Location location = this->cursor.empty() ? memory::Location{} : this->cursor.lookahead(0).location;
            const std::string message = "Dangling scope pop boundary constraint violation";
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Error, "Scope pop failure at {}:{}", location.line, location.column);
            this->history_.emplace_back(Traceback::Type::Scope, location, message);
            return;
        }

        this->state_.pop();

        const std::size_t mark = this->marks.back();
        this->marks.pop_back();

        for (std::size_t count = this->records.size(); count > mark; --count) {
            Record record = std::move(this->records.back());
            this->records.pop_back();
            if (record.symbol < this->macros.size()) {
                this->macros[record.symbol] = std::move(record.macro);
                this->macros[record.symbol].active = record.active;
            }
        }

        if (!this->deferred.empty()) {
            std::vector<Token> pending = std::move(this->deferred.back());
            this->deferred.pop_back();
            if (!pending.empty()) {
                this->inject(pending);
            }
        }
    }

    void Mouth::globalize() noexcept {
        this->global = 1;
    }

    int Mouth::unglobal() noexcept {
        const int flag = this->global;
        this->global = 0;
        return flag;
    }

    void Mouth::longify() noexcept {
        this->spanning = 1;
    }

    int Mouth::unlong() noexcept {
        const int flag = this->spanning;
        this->spanning = 0;
        return flag;
    }

    void Mouth::outerize() noexcept {
        this->isolated = 1;
    }

    int Mouth::unouter() noexcept {
        const int flag = this->isolated;
        this->isolated = 0;
        return flag;
    }

    void Mouth::defer(const Token& token) {
        if (!this->deferred.empty()) {
            this->deferred.back().push_back(token);
        }
    }

    void Mouth::schedule(const Token& token) {
        this->scheduled = token;
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

    int Mouth::step() {
        if (this->cursor.empty()) return 0;

        const Token token = this->cursor.lookahead(0);
        if (token.symbol == kInvalidSymbol) return 0;

        if (this->suppressed) {
            this->suppressed = 0;
            return 0;
        }

        if (token.symbol < this->handlers.size() && this->handlers[token.symbol]) {
            this->cursor.advance();
            this->handlers[token.symbol](*this);
            return 1;
        }

        if (token.symbol < this->macros.size() && this->macros[token.symbol].active) {
            this->cursor.advance();

            if (this->depth >= this->limit) {
                const std::string message = std::format("Macro recursion threshold {} exceeded", this->limit);
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Error, "Recursion limit hit at {}:{}", token.location.line, token.location.column);
                this->history_.emplace_back(Traceback::Type::Recursion, token.location, message);
                return 0;
            }
            this->depth++;

            const auto& macro = this->macros[token.symbol];
            std::vector<std::vector<Token>> arguments;
            arguments.reserve(macro.parameters.size());
            for (const auto& parameter : macro.parameters) {
                arguments.push_back(this->argument(parameter, macro.spanning));
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
            return 1;
        }

        return 0;
    }

    std::vector<Token> Mouth::argument(const Parameter& parameter, const int allow) {
        std::vector<Token> result;
        if (this->cursor.empty()) return result;

        const auto forbidden = [&](const Token& token) -> int {
            if (token.symbol == this->paragraph && !allow) {
                const std::string message = "Paragraph break inside argument of a macro that isn't \\long";
                Logger::log(Logger::Type::Mouth, Logger::Level::Warning, message);
                this->history_.emplace_back(Traceback::Type::Argument, token.location, message);
                return 1;
            }
            if (const auto slot = static_cast<std::size_t>(token.symbol); slot < this->macros.size() && this->macros[slot].active && this->macros[slot].isolated) {
                const std::string message = std::format("\\outer macro {} may not appear inside an argument", token.values);
                Logger::log(Logger::Type::Mouth, Logger::Level::Warning, message);
                this->history_.emplace_back(Traceback::Type::Argument, token.location, message);
                return 1;
            }
            return 0;
        };

        if (parameter.optional) {
            const Token lead = this->cursor.lookahead(0);
            if (lead.category == CatCodes::Category::Other && lead.values == "[") {
                this->cursor.advance();
                std::size_t scope = 1;
                while (!this->cursor.empty() && scope > 0) {
                    Token token = this->cursor.advance();
                    if (token.values == "[") scope++;
                    else if (token.values == "]") scope--;
                    if (scope > 0) {
                        if (forbidden(token)) break;
                        result.push_back(token);
                    }
                }
            } else {
                result = parameter.fallbacks;
            }
        } else if (!parameter.delimiters.empty()) {
            std::size_t scope = 0;
            bool matched = false;

            while (!this->cursor.empty()) {
                if (scope == 0) {
                    if (const auto span = probe(this->cursor, parameter.delimiters)) {
                        matched = true;
                        for (std::size_t count = 0; count < *span; ++count) {
                            this->cursor.advance();
                        }
                        break;
                    }
                }

                Token token = this->cursor.advance();
                if (forbidden(token)) break;

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
                this->history_.emplace_back(Traceback::Type::Delimiter, location, message);
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
                    if (scope > 0) {
                        if (forbidden(element)) break;
                        result.push_back(element);
                    }
                }
                if (scope > 0) {
                    const std::string message = "Macro argument block truncation missing trailing brace";
                    Logger::log(Logger::Type::Mouth, Logger::Level::Warning, message);
                    this->history_.emplace_back(Traceback::Type::Group, token.location, message);
                }
            } else if (forbidden(token)) {
                this->inject(std::span{&token, 1});
            } else if (token.symbol != kInvalidSymbol || !token.values.empty()) {
                result.push_back(token);
            }
        }
        return result;
    }

    void Mouth::inject(const std::span<const Token> tokens) {
        this->cursor.inject(tokens);
    }

    void Mouth::ingest(const std::string_view source) {
        Lexer lexer(source, this->state_.catcodes(), this->lexicon_);
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
        if (symbol >= this->handlers.size()) {
            this->handlers.resize(std::max<std::size_t>(needed, this->handlers.size() * 2));
        }
        this->handlers[symbol] = std::move(handler);
    }

    void Mouth::define(const std::string_view name, Macro macro) {
        this->define(this->lexicon_.intern(name), std::move(macro));
    }

    void Mouth::define(const Symbol symbol, Macro macro) {
        const auto slot = static_cast<std::size_t>(symbol);
        const int is_global = this->unglobal();

        if (!is_global && !this->marks.empty()) {
            const bool is_active = slot < this->macros.size() && this->macros[slot].active;
            const Macro previous = is_active ? this->macros[slot] : Macro{};
            this->records.push_back(Record{symbol, previous, is_active});
        }

        if (slot >= this->macros.size()) {
            this->macros.resize(std::max<std::size_t>(slot + 1, this->macros.size() * 2));
        }
        macro.active = true;
        this->macros[slot] = std::move(macro);

        if (this->scheduled) {
            const Token pending = *this->scheduled;
            this->scheduled.reset();
            this->inject(std::span{&pending, 1});
        }
    }

    void Mouth::undefine(const std::string_view name) {
        this->undefine(this->lexicon_.intern(name));
    }

    void Mouth::undefine(const Symbol symbol) {
        const int is_global = this->unglobal();
        if (const auto slot = static_cast<std::size_t>(symbol); slot < this->macros.size() && this->macros[slot].active) {
            if (!is_global && !this->marks.empty()) {
                this->records.push_back(Record{symbol, this->macros[slot], true});
            }
            this->macros[slot].active = false;
        }

        if (this->scheduled) {
            const Token pending = *this->scheduled;
            this->scheduled.reset();
            this->inject(std::span{&pending, 1});
        }
    }

    Token Mouth::lookahead(const std::size_t offset) const noexcept {
        return this->cursor.lookahead(offset);
    }

    std::optional<Mouth::Macro> Mouth::lookup(const Symbol symbol) const noexcept {
        const auto slot = static_cast<std::size_t>(symbol);
        if (slot < this->macros.size() && this->macros[slot].active) {
            return this->macros[slot];
        }
        return std::nullopt;
    }

    Mouth::Handler Mouth::primitive(const Symbol symbol) const noexcept {
        const auto slot = static_cast<std::size_t>(symbol);
        if (slot < this->handlers.size()) {
            return this->handlers[slot];
        }
        return {};
    }

    std::optional<std::int32_t> Mouth::integer(const semantics::Registers& registers, const Symbol count) {
        return Number::integer(this->cursor, registers, count);
    }

    std::optional<std::int32_t> Mouth::dimension(const semantics::Registers& registers, const Symbol count, const Symbol unit) {
        return Number::dimension(this->cursor, registers, count, unit);
    }

}