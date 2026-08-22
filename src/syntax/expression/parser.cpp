#include "syntax/expression/parser.hpp"
#include "logger.hpp"

#include <cctype>

namespace syntax::expression {

    Parser::Parser(Mouth& mouth, const Unicodes& unicodes, memory::Arena& arena, const Node::Style style)
        : mouth(mouth), unicodes(unicodes), arena(arena), style(style) {
        current = mouth.expand();
    }

    Token Parser::advance() {
        const Token previous = current;
        current = mouth.expand();
        return previous;
    }

    Token Parser::lookahead() const noexcept {
        return current;
    }

    Node* Parser::compose(const Node::Type type) const {
        auto* node = arena.compose<Node>(type);
        node->style = style;
        return node;
    }

    void Parser::bind(const Symbol symbol, const Node::Type type, const int weight, const bool right, const bool structural) {
        const auto index = static_cast<std::size_t>(symbol);
        if (index >= rules.size()) {
            rules.resize(index + 1, Rule{Node::Type::Variable, 0, false, false});
        }
        rules[index] = Rule{type, weight, right, structural};
        Logger::fmt(Logger::Type::Parser, Logger::Level::Debug, "Bound symbol {} to rule type={}, weight={}, right={}, structural={}",
                    index, static_cast<int>(type), weight, right, structural);
    }

    Node* Parser::sequence(const char closing) {
        if (depth >= limit) {
            const memory::Location location = current.location;
            Logger::fmt(Logger::Type::Parser, Logger::Level::Error, "Recursion limit threshold hit at {}:{}", location.line, location.column);
            return nullptr;
        }

        struct Guard {
            std::size_t& depth;
            explicit Guard(std::size_t& depth) : depth(depth) { ++depth; }
            ~Guard() { --depth; }
        } guard(depth);

        std::vector<Node*> items;

        while (true) {
            const Token next = lookahead();
            if (next.values.empty()) {
                if (closing != 0) {
                    Logger::fmt(Logger::Type::Parser, Logger::Level::Warning, "Unexpected EOF while waiting for closing delimiter '{}' at {}:{}",
                                closing, next.location.line, next.location.column);
                }
                break;
            }

            if (closing != 0 && next.values.size() == 1 && next.values[0] == closing) {
                advance();
                break;
            }

            if (closing == 0 && next.values.size() == 1 && (next.values[0] == '}' || next.values[0] == ')')) {
                break;
            }

            auto* item = step(0);
            if (!item) {
                break;
            }
            items.push_back(item);
        }

        if (items.empty()) {
            return nullptr;
        }
        if (items.size() == 1) {
            return items[0];
        }

        auto* node = compose(Node::Type::Sequence);
        auto slice = arena.allocate<Node*>(items.size());
        for (std::size_t index = 0; index < items.size(); ++index) {
            slice[index] = items[index];
        }
        node->arguments = slice;
        return node;
    }

    Node* Parser::group(const char closing) {
        auto* node = compose(Node::Type::Group);
        node->left = sequence(closing);
        return node;
    }

    Node* Parser::command(const Token& token) {
        auto* node = compose(Node::Type::Sequence);
        node->value = token.values;

        if (lookahead().values == "[") {
            advance();
            node->subscript = sequence(']');
        }

        std::vector<Node*> parameters;
        while (true) {
            const Token next = lookahead();
            if (next.values.empty()) break;

            if (next.values == "{") {
                advance();
                parameters.push_back(sequence('}'));
            } else if (next.category == CatCodes::Category::Escape ||
                       (next.values.size() == 1 && std::isalnum(static_cast<unsigned char>(next.values[0])))) {
                parameters.push_back(core());
            } else {
                break;
            }
        }

        if (!parameters.empty()) {
            auto slice = arena.allocate<Node*>(parameters.size());
            for (std::size_t index = 0; index < parameters.size(); ++index) {
                slice[index] = parameters[index];
            }
            node->arguments = slice;
            if (!parameters.empty()) node->left = parameters[0];
            if (parameters.size() >= 2) node->right = parameters[1];
        }

        return node;
    }

    Node* Parser::structural(const Node::Type type) {
        auto* node = compose(type);

        if (type == Node::Type::Radical) {
            if (lookahead().values == "[") {
                advance();
                node->left = sequence(']');
            }
            node->right = core();
        } else if (type == Node::Type::Fraction) {
            node->left = core();
            node->right = core();
        } else if (type == Node::Type::Accent) {
            node->left = core();
        }

        return node;
    }

    Node* Parser::core() {
        const Token token = advance();
        if (token.values.empty()) {
            return nullptr;
        }

        if (token.values == "{") return group('}');
        if (token.values == "(") return group(')');

        if (const auto index = static_cast<std::size_t>(token.symbol); index < rules.size() && rules[index].structural) {
            return structural(rules[index].type);
        }

        std::string_view name = token.values;
        if (name.starts_with('\\')) {
            name.remove_prefix(1);
        }

        if (const auto symbol = unicodes.query(name)) {
            auto* node = compose(Node::Type::Variable);
            node->value = token.values;
            node->codepoint = symbol->codepoint;
            node->category = symbol->category;
            return node;
        }

        if (token.category == CatCodes::Category::Escape || token.values.starts_with('\\')) {
            return command(token);
        }

        auto* node = compose(Node::Type::Variable);
        node->value = token.values;
        return node;
    }

    Node* Parser::atom() {
        const Token next = lookahead();

        if (const auto index = static_cast<std::size_t>(next.symbol); index < rules.size() && rules[index].type == Node::Type::Unary && rules[index].weight == 0) {
            const Token token = advance();
            auto* unaryNode = compose(Node::Type::Unary);
            unaryNode->value = token.values;
            unaryNode->left = atom();
            return script(unaryNode);
        }

        return script(core());
    }

    Node* Parser::script(Node* base) {
        if (!base) return nullptr;

        Node* subscript = nullptr;
        Node* superscript = nullptr;

        while (true) {
            if (const Token next = lookahead(); next.values == "_") {
                advance();
                subscript = core();
            } else if (next.values == "^") {
                advance();
                superscript = core();
            } else {
                break;
            }
        }

        if (!subscript && !superscript) {
            return base;
        }

        auto* node = compose(Node::Type::Script);
        node->left = base;
        node->subscript = subscript;
        node->superscript = superscript;
        return node;
    }

    Node* Parser::step(const int priority) {
        Node* left = atom();
        if (!left) return nullptr;

        while (true) {
            const Token next = lookahead();
            if (next.values.empty() || next.values == "}" || next.values == ")" || next.values == "]") {
                break;
            }

            const auto index = static_cast<std::size_t>(next.symbol);
            Rule rule{Node::Type::Variable, 0, false, false};
            if (index < rules.size()) {
                rule = rules[index];
            }

            if (rule.type == Node::Type::Unary && rule.weight > 0) {
                if (rule.weight < priority) {
                    break;
                }
                const Token token = advance();
                auto* postfix = compose(Node::Type::Unary);
                postfix->value = token.values;
                postfix->left = left;
                left = script(postfix);
                continue;
            }

            if (rule.weight > 0 && !rule.structural) {
                if (rule.weight < priority || (rule.weight == priority && !rule.right)) {
                    break;
                }

                const Token token = advance();
                auto* binary = compose(rule.type);
                binary->value = token.values;
                binary->left = left;
                binary->right = step(rule.weight);
                left = script(binary);
            } else {
                break;
            }
        }

        return left;
    }

    Node* Parser::parse() {
        Logger::fmt(Logger::Type::Parser, Logger::Level::Debug, "Starting AST generation stream");
        Node* root = sequence(0);
        if (!root) {
            Logger::log(Logger::Type::Parser, Logger::Level::Warning, "AST generation completed with empty or null root node");
        } else {
            Logger::fmt(Logger::Type::Parser, Logger::Level::Debug, "AST root constructed successfully with type {}", static_cast<int>(root->type));
        }
        return root;
    }

}