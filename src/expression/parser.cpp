#include "expression/parser.hpp"

namespace expression {

    Parser::Parser(syntax::Mouth& mouth, const syntax::Unicodes& unicodes, memory::Arena& arena)
        : mouth(mouth), unicodes(unicodes), arena(arena) {
        current = mouth.expand();
    }

    syntax::Token Parser::advance() {
        const syntax::Token previous = current;
        current = mouth.expand();
        return previous;
    }

    syntax::Token Parser::lookahead() const noexcept {
        return current;
    }

    Node* Parser::compose(const Node::Type type) const {
        auto slice = arena.allocate<Node>(1);
        Node* node = &slice[0];
        node->type = type;
        node->type_ = syntax::Unicodes::Type::Ordinary;
        node->codepoint = 0;
        node->value = {};
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    void Parser::bind(const syntax::Symbol symbol, const Node::Type type, const int weight, const bool right, const bool structural) {
        const auto index = static_cast<std::size_t>(symbol);
        if (index >= rules.size()) {
            rules.resize(index + 1, Rule{Node::Type::Variable, 0, false, false});
        }
        rules[index] = Rule{type, weight, right, structural};
    }

    Node* Parser::structural(const Node::Type type) {
        auto* node = compose(type);
        if (type == Node::Type::Fraction) {
            node->left = core();
            node->right = core();
        } else if (type == Node::Type::Radical) {
            node->right = core();
        }
        return node;
    }

    Node* Parser::core() {
        const syntax::Token token = advance();
        if (token.value.empty()) {
            return nullptr;
        }

        if (token.value == "{") return group('}');
        if (token.value == "(") return group(')');

        if (const auto index = static_cast<std::size_t>(token.symbol); index < rules.size() && rules[index].structural) {
            return structural(rules[index].type);
        }

        std::string_view name = token.value;
        if (name.starts_with('\\')) {
            name.remove_prefix(1);
        }

        if (const auto symbol = unicodes.query(name)) {
            auto* node = compose(Node::Type::Variable);
            node->value = token.value;
            node->codepoint = symbol->codepoint;
            node->type_ = symbol->type;
            return node;
        }

        auto* node = compose(Node::Type::Variable);
        node->value = token.value;
        return node;
    }

    Node* Parser::atom() {
        return script(core());
    }

    Node* Parser::group(const char closing) {
        auto* node = compose(Node::Type::Group);
        node->left = step(0);

        if (lookahead().value.size() == 1 && lookahead().value[0] == closing) {
            advance();
        }

        return node;
    }

    Node* Parser::script(Node* base) {
        if (!base) return nullptr;

        while (true) {
            if (const syntax::Token next = lookahead(); next.value == "_") {
                advance();
                auto* node = compose(Node::Type::Subscript);
                node->left = base;
                node->right = core();
                base = node;
            } else if (next.value == "^") {
                advance();
                auto* node = compose(Node::Type::Superscript);
                node->left = base;
                node->right = core();
                base = node;
            } else {
                break;
            }
        }
        return base;
    }

    Node* Parser::step(const int priority) {
        Node* left = atom();
        if (!left) return nullptr;

        while (true) {
            const syntax::Token next = lookahead();
            if (next.value.empty() || next.value == "}" || next.value == ")") {
                break;
            }

            const auto index = static_cast<std::size_t>(next.symbol);
            Rule rule{Node::Type::Variable, 0, false, false};
            if (index < rules.size()) {
                rule = rules[index];
            }

            if (rule.weight > 0 && !rule.structural) {
                if (rule.weight < priority || (rule.weight == priority && !rule.right)) {
                    break;
                }

                const syntax::Token token = advance();
                auto* binary = compose(rule.type);
                binary->value = token.value;
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
        return step(0);
    }

}