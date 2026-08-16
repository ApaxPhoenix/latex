#include "syntax/mouth.hpp"
#include "syntax/lexer.hpp"
#include "syntax/number.hpp"
#include "memory/location.hpp"
#include "logger.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

namespace syntax {

    struct Binding {
        std::string_view name;
        Mouth::Handler handler;
    };

    static void skip(Mouth& mouth, const bool check, const Symbol otherwise, const Symbol end) {
        Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "Entering conditional block skip pass");
        std::size_t depth = 1;
        Token token;
        while (!(token = mouth.read()).value.empty() && depth > 0) {
            if (token.type == CatCodes::Type::Escape) {
                if (token.value.starts_with("\\if")) {
                    depth++;
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                                "Nested conditional encountered during skip (depth={})", depth);
                } else if (token.symbol == end) {
                    depth--;
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                                "Closing \\fi encountered during skip (depth={})", depth);
                } else if (check && token.symbol == otherwise && depth == 1) {
                    Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Matching \\else branch hit at depth 1");
                    break;
                }
            }
        }

        if (depth > 0) {
            const std::string message = "Unterminated conditional block (missing \\fi before end of input)";
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Error,
                        "Conditional skip error at line {} col {}: {}", token.location.line, token.location.column, message);
            mouth.tracebacks().emplace_back(
                Traceback::Type::Syntax,
                token.location,
                message
            );
        }
    }

    Mouth::Mouth(Cursor cursor, semantics::Union& state, Lexicon& lexicon, memory::Arena& arena)
        : cursor_(std::move(cursor)), state_(state), lexicon_(lexicon), arena_(arena) {

        count_ = lexicon_.intern("\\count_");
        dimension_ = lexicon_.intern("\\dimen");
        else_ = lexicon_.intern("\\else");
        fi_ = lexicon_.intern("\\fi");

        const Symbol open = lexicon_.intern("{");
        const Symbol close = lexicon_.intern("}");

        this->bind(open, [](Mouth& mouth) {
            Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Opening group scope '{'");
            mouth.state().push(semantics::Scope::Type::Group);
            mouth.push();
        });

        this->bind(close, [](Mouth& mouth) {
            Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Closing group scope '}'");
            mouth.state().pop();
            mouth.pop();
        });

        static const Binding builtins[] = {
            {"\\begingroup", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "Executing \\begingroup directive");
                mouth.state().push(semantics::Scope::Type::Group);
                mouth.push();
            }},
            {"\\endgroup", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "Executing \\endgroup directive");
                mouth.state().pop();
                mouth.pop();
            }},
            {"\\global", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "Setting next assignment scope to global");
                mouth.globalize();
            }},
            {"\\catcode", [](Mouth& mouth) {
                const Token target = mouth.read();
                if (mouth.cursor_.lookahead(0).value == "=") mouth.read();
                if (const auto code = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_)) {
                    if (!target.value.empty()) {
                        Logger::fmt(Logger::Type::Mouth, Logger::Level::Informative,
                                    "Assigning catcode for character '{}' to {}", target.value[0], *code);
                        mouth.state().catcodes().set(target.value[0], static_cast<CatCodes::Type>(*code));
                    }
                } else {
                    const std::string message = "Invalid category code value in \\catcode assignment";
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Warning, "Catcode error: {}", message);
                    mouth.tracebacks().emplace_back(
                        Traceback::Type::Catcode,
                        target.location,
                        message
                    );
                }
            }},
            {"\\def", [](Mouth& mouth) {
                const Token name = mouth.read();
                if (name.type != CatCodes::Type::Escape) {
                    const std::string message = "Expected control sequence macro name following \\def";
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Warning, "Macro definition error: {}", message);
                    mouth.tracebacks().emplace_back(
                        Traceback::Type::Macro,
                        name.location,
                        message
                    );
                    return;
                }

                Mouth::Macro macro;
                while (!mouth.cursor_.empty() && mouth.cursor_.lookahead(0).value != "{") {
                    Token token = mouth.read();
                    if (token.type == CatCodes::Type::Parameter) {
                        Mouth::Parameter param;
                        while (!mouth.cursor_.empty() &&
                               mouth.cursor_.lookahead(0).value != "{" &&
                               mouth.cursor_.lookahead(0).type != CatCodes::Type::Parameter) {
                            param.delimiter.push_back(mouth.read());
                        }
                        macro.parameters.push_back(std::move(param));
                    }
                }

                if (mouth.cursor_.empty() || mouth.cursor_.lookahead(0).value != "{") {
                    const std::string message = "Missing replacement equations body block '{...}' in \\def definition";
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Warning, "Macro definition error: {}", message);
                    mouth.tracebacks().emplace_back(
                        Traceback::Type::Syntax,
                        name.location,
                        message
                    );
                    return;
                }

                macro.body = mouth.argument({});
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Informative,
                            "Defined macro '{}' (text={}) with {} parameters and {} body tokens",
                            name.value, name.symbol, macro.parameters.size(), macro.body.size());
                mouth.define(name.symbol, std::move(macro));
            }},
            {"\\let", [](Mouth& mouth) {
                const Token target = mouth.read();
                if (mouth.cursor_.lookahead(0).value == "=") mouth.read();
                if (const Token source = mouth.read(); source.symbol < mouth.primitives_.size() && mouth.primitives_[source.symbol]) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Aliasing primitive text {} -> {}", source.symbol, target.symbol);
                    mouth.bind(target.symbol, mouth.primitives_[source.symbol]);
                } else if (source.symbol < mouth.macros_.size() && mouth.macros_[source.symbol].active) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Aliasing macro text {} -> {}", source.symbol, target.symbol);
                    mouth.define(target.symbol, mouth.macros_[source.symbol]);
                }
            }},
            {"\\expandafter", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Executing \\expandafter expansion");
                Token token = mouth.read();
                std::ignore = mouth.step();
                mouth.inject(std::span<const Token>(&token, 1));
            }},
            {"\\noexpand", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Suppressing expansion via \\noexpand");
                mouth.suppress_ = true;
            }},
            {"\\csname", [](Mouth& mouth) {
                Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "Constructing control sequence via \\csname");
                std::string buffer;
                while (!mouth.cursor_.empty()) {
                    const Token token = mouth.expand();
                    if (token.value == "\\endcsname") break;
                    buffer += token.value;
                }
                const std::string text = "\\" + buffer;
                const std::string_view copy = mouth.arena_.copy(text);
                const Symbol symbol = mouth.lexicon_.intern(copy);
                Token result{symbol, copy, {}, CatCodes::Type::Escape};
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                            "Constructed macro name '{}' via \\csname", text);
                mouth.inject(std::span<const Token>(&result, 1));
            }},
            {"\\endcsname", [](Mouth&) {}},

            {"\\countdef", [](Mouth& mouth) {
                const Token name = mouth.read();
                if (mouth.cursor_.lookahead(0).value == "=") mouth.read();
                if (const auto slot = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_)) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Bound count register alias text {} to index {}", name.symbol, *slot);
                    mouth.state_.registers().bind(name.symbol, semantics::Registers::Type::Count, static_cast<std::size_t>(*slot));
                }
            }},
            {"\\dimendef", [](Mouth& mouth) {
                const Token name = mouth.read();
                if (mouth.cursor_.lookahead(0).value == "=") mouth.read();
                if (const auto slot = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_)) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Bound dimension register alias text {} to index {}", name.symbol, *slot);
                    mouth.state_.registers().bind(name.symbol, semantics::Registers::Type::Dimension, static_cast<std::size_t>(*slot));
                }
            }},
            {"\\count_", [](Mouth& mouth) {
                const auto slot = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_);
                if (mouth.cursor_.lookahead(0).value == "=") mouth.read();
                if (const auto value = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_); slot && value) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Assigned register count slot {} = {}", *slot, *value);
                    mouth.state_.registers().assign(semantics::Registers::Type::Count, static_cast<std::size_t>(*slot), *value, mouth.global_);
                    mouth.global_ = false;
                }
            }},
            {"\\advance", [](Mouth& mouth) {
                const Token target = mouth.read();
                if (mouth.cursor_.lookahead(0).value == "\\by") mouth.read();
                if (const auto amount = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_)) {
                    const auto current = mouth.state_.registers().get(target.symbol);
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "Advanced register text {} by {} (new value: {})", target.symbol, *amount, current + *amount);
                    mouth.state_.registers().set(target.symbol, current + *amount, mouth.global_);
                    mouth.global_ = false;
                }
            }},

            {"\\ifx", [](Mouth& mouth) {
                const Token first = mouth.read();
                if (const Token second = mouth.read(); first.symbol != second.symbol || (first.type != second.type)) {
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                                "\\ifx comparison failed ('{}' vs '{}'), skipping condition body", first.value, second.value);
                    skip(mouth, true, mouth.else_, mouth.fi_);
                }
            }},
            {"\\ifnum", [](Mouth& mouth) {
                const auto first = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_);
                const Token operator_ = mouth.read();
                const auto second = Number::integer(mouth.cursor_, mouth.state_.registers(), mouth.count_);
                bool pass = false;
                if (first && second) {
                    if (operator_.value == "<") pass = (*first < *second);
                    else if (operator_.value == "=") pass = (*first == *second);
                    else if (operator_.value == ">") pass = (*first > *second);
                }
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                            "\\ifnum evaluated result: {}", pass);
                if (!pass) skip(mouth, true, mouth.else_, mouth.fi_);
            }},
            {"\\ifdim", [](Mouth& mouth) {
                const auto first = Number::dimension(mouth.cursor_, mouth.state_.registers(), mouth.count_, mouth.dimension_);
                const Token operator_ = mouth.read();
                const auto second = Number::dimension(mouth.cursor_, mouth.state_.registers(), mouth.count_, mouth.dimension_);
                bool pass = false;
                if (first && second) {
                    if (operator_.value == "<") pass = (*first < *second);
                    else if (operator_.value == "=") pass = (*first == *second);
                    else if (operator_.value == ">") pass = (*first > *second);
                }
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                            "\\ifdim evaluated result: {}", pass);
                if (!pass) skip(mouth, true, mouth.else_, mouth.fi_);
            }},
            {"\\iftrue",  [](Mouth&) { Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "\\iftrue taking true branch"); }},
            {"\\iffalse", [](Mouth& mouth) { Logger::log(Logger::Type::Mouth, Logger::Level::Debug, "\\iffalse taking false branch"); skip(mouth, true, mouth.else_, mouth.fi_); }},
            {"\\or",      [](Mouth& mouth) { skip(mouth, false, mouth.else_, mouth.fi_); }},
            {"\\else",    [](Mouth& mouth) { skip(mouth, false, mouth.else_, mouth.fi_); }},
            {"\\fi",      [](Mouth&) { Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Encountered \\fi boundary"); }},
            {"\\relax",   [](Mouth&) { Logger::log(Logger::Type::Mouth, Logger::Level::Traceback, "Executing \\relax NOP"); }}
        };

        for (const auto& [name, handler] : builtins) {
            this->bind(name, handler);
        }
        Logger::log(Logger::Type::Mouth, Logger::Level::Informative, "Mouth initialized with core system primitives");
    }

    void Mouth::push() {
        this->marks_.push_back(this->undo_.size());
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                    "Pushed scope mark at depth {}", this->marks_.size());
    }

    void Mouth::pop() {
        if (this->marks_.empty()) {
            const memory::Location location = this->cursor_.empty() ? memory::Location{} : this->cursor_.lookahead(0).location;
            const std::string message = "Unmatched closing group brace '}' or \\endgroup directive";
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Error,
                        "Scope error at line {} col {}: {}", location.line, location.column, message);
            this->tracebacks_.emplace_back(
                Traceback::Type::Scope,
                location,
                message
            );
            return;
        }
        const std::size_t mark = this->marks_.back();
        this->marks_.pop_back();

        for (std::size_t count = this->undo_.size(); count > mark; --count) {
            const auto [symbol, macro, active] = this->undo_.back();
            this->undo_.pop_back();
            if (symbol < this->macros_.size()) {
                this->macros_[symbol] = macro;
                this->macros_[symbol].active = active;
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                            "Restored macro definition state for text {} on scope exit", symbol);
            }
        }
    }

    void Mouth::globalize() noexcept {
        this->global_ = true;
    }

    Token Mouth::read() noexcept {
        if (this->cursor_.empty()) return {};
        return this->cursor_.advance();
    }

    Token Mouth::expand() {
        while (!this->cursor_.empty()) {
            if (!this->step()) return this->cursor_.advance();
        }
        return {};
    }

    bool Mouth::step() {
        if (this->cursor_.empty()) return false;

        const Token token = this->cursor_.lookahead(0);
        if (token.symbol == kInvalidSymbol) return false;

        if (this->suppress_) {
            this->suppress_ = false;
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                        "Expansion suppressed for token '{}'", token.value);
            return false;
        }

        if (token.symbol < this->primitives_.size() && this->primitives_[token.symbol]) {
            this->cursor_.advance();
            Logger::fmt(Logger::Type::Mouth, Logger::Level::Traceback,
                        "Executing primitive handler for command '{}' (text={})", token.value, token.symbol);
            this->primitives_[token.symbol](*this);
            return true;
        }

        if (token.symbol < this->macros_.size() && this->macros_[token.symbol].active) {
            this->cursor_.advance();

            if (this->depth_ >= this->limit_) {
                const std::string message = "Macro expansion recursion limit exceeded (" + std::to_string(this->limit_) + ")";
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Error,
                            "Recursion error at line {} col {}: {}", token.location.line, token.location.column, message);
                this->tracebacks_.emplace_back(
                    Traceback::Type::Recursion,
                    token.location,
                    message
                );
                return false;
            }
            this->depth_++;

            Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                        "Expanding macro '{}' (depth={}/{})", token.value, this->depth_, this->limit_);

            const auto& macro = this->macros_[token.symbol];
            std::vector<std::vector<Token>> arguments;
            arguments.reserve(macro.parameters.size());
            for (const auto& parameter : macro.parameters) {
                arguments.push_back(this->argument(parameter));
            }

            std::vector<Token> output;
            const auto& body = macro.body;
            output.reserve(body.size());

            for (std::size_t index = 0; index < body.size(); ++index) {
                if (body[index].type == CatCodes::Type::Parameter && index + 1 < body.size()) {
                    if (const auto& value = body[index + 1].value; !value.empty() && value[0] >= '1' && value[0] <= '9') {
                        if (const auto slot = static_cast<std::size_t>(value[0] - '1'); slot < arguments.size()) {
                            output.insert(output.end(), arguments[slot].begin(), arguments[slot].end());
                        }
                        index++;
                        continue;
                    }
                }
                output.push_back(body[index]);
            }

            this->cursor_.inject(output);
            this->depth_--;
            return true;
        }

        return false;
    }

    std::vector<Token> Mouth::argument(const Parameter& parameter) {
        std::vector<Token> result;
        if (this->cursor_.empty()) return result;

        if (parameter.optional) {
            if (this->cursor_.lookahead().type == CatCodes::Type::Other && this->cursor_.lookahead().value == "[") {
                this->cursor_.advance();
                std::size_t scope = 1;
                while (!this->cursor_.empty() && scope > 0) {
                    Token token = this->cursor_.advance();
                    if (token.value == "[") scope++;
                    else if (token.value == "]") scope--;
                    if (scope > 0) result.push_back(token);
                }
            } else {
                result = parameter.fallback;
            }
        } else if (!parameter.delimiter.empty()) {
            std::size_t scope = 0;
            bool matched = false;
            while (!this->cursor_.empty()) {
                if (scope == 0) {
                    matched = true;
                    for (std::size_t index = 0; index < parameter.delimiter.size(); ++index) {
                        if (this->cursor_.lookahead(index).symbol != parameter.delimiter[index].symbol) {
                            matched = false;
                            break;
                        }
                    }
                    if (matched) {
                        for (std::size_t index = 0; index < parameter.delimiter.size(); ++index) {
                            this->cursor_.advance();
                        }
                        break;
                    }
                }

                Token token = this->cursor_.advance();
                if (token.type == CatCodes::Type::Group) {
                    if (token.value == "{") scope++;
                    else if (token.value == "}" && scope > 0) scope--;
                }
                result.push_back(token);
            }
            if (!matched) {
                const memory::Location location = this->cursor_.empty() ? memory::Location{} : this->cursor_.lookahead(0).location;
                const std::string message = "Delimited macro argument missing matching trailing boundary";
                Logger::fmt(Logger::Type::Mouth, Logger::Level::Warning, "Argument parsing error: {}", message);
                this->tracebacks_.emplace_back(
                    Traceback::Type::Delimiter,
                    location,
                    message
                );
            }
        } else {
            while (!this->cursor_.empty() && this->cursor_.lookahead(0).type == CatCodes::Type::Space) {
                this->cursor_.advance();
            }

            if (const Token token = this->cursor_.advance(); token.type == CatCodes::Type::Group && token.value == "{") {
                std::size_t scope = 1;
                while (!this->cursor_.empty() && scope > 0) {
                    Token element = this->cursor_.advance();
                    if (element.type == CatCodes::Type::Group) {
                        if (element.value == "{") scope++;
                        else if (element.value == "}") scope--;
                    }
                    if (scope > 0) result.push_back(element);
                }
                if (scope > 0) {
                    const std::string message = "Unclosed group brace '{' while parsing macro argument block";
                    Logger::fmt(Logger::Type::Mouth, Logger::Level::Warning,
                                "Argument error at line {} col {}: {}", token.location.line, token.location.column, message);
                    this->tracebacks_.emplace_back(
                        Traceback::Type::Group,
                        token.location,
                        message
                    );
                }
            } else {
                result.push_back(token);
            }
        }
        return result;
    }

    void Mouth::inject(const std::span<const Token> tokens) {
        this->cursor_.inject(tokens);
    }

    void Mouth::ingest(const std::string_view source) {
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Informative,
                    "Ingesting input source buffer size: {} bytes", source.size());
        Lexer lexer(source, this->state_.catcodes(), this->lexicon_);
        std::vector<Token> tokens;
        tokens.reserve(source.size() / 2);
        while (!lexer.empty()) {
            if (Token token = lexer.advance(); !token.value.empty()) {
                tokens.push_back(token);
            }
        }
        Logger::fmt(Logger::Type::Mouth, Logger::Level::Debug,
                    "Ingestion lexed {} tokens into cursor stream", tokens.size());
        this->cursor_.inject(tokens);
    }

    void Mouth::bind(const std::string_view name, Handler handler) {
        this->bind(this->lexicon_.intern(name), std::move(handler));
    }

    void Mouth::bind(const Symbol symbol, Handler handler) {
        const auto needed = static_cast<std::size_t>(symbol) + 1;
        if (symbol >= this->primitives_.size()) {
            this->primitives_.resize(std::max<std::size_t>(needed, this->primitives_.size() * 2));
        }
        this->primitives_[symbol] = std::move(handler);
    }

    void Mouth::define(const std::string_view name, Macro macro) {
        this->define(this->lexicon_.intern(name), std::move(macro));
    }

    void Mouth::define(const Symbol symbol, Macro macro) {
        const auto slot = static_cast<std::size_t>(symbol);

        if (!this->global_ && !this->marks_.empty()) {
            const bool active = (slot < this->macros_.size() && this->macros_[slot].active);
            const Macro previous = active ? this->macros_[slot] : Macro{};
            this->undo_.push_back(Record{symbol, previous, active});
        }

        if (slot >= this->macros_.size()) {
            this->macros_.resize(std::max<std::size_t>(slot + 1, this->macros_.size() * 2));
        }
        macro.active = true;
        this->macros_[slot] = std::move(macro);
        this->global_ = false;
    }

    void Mouth::undefine(const std::string_view name) {
        this->undefine(this->lexicon_.intern(name));
    }

    void Mouth::undefine(const Symbol symbol) {
        if (const auto slot = static_cast<std::size_t>(symbol); slot < this->macros_.size() && this->macros_[slot].active) {
            if (!this->global_ && !this->marks_.empty()) {
                this->undo_.push_back(Record{symbol, this->macros_[slot], true});
            }
            this->macros_[slot].active = false;
        }
        this->global_ = false;
    }

}