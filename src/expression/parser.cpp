#include "expression/parser.hpp"

namespace expression {

    Parser::Parser(syntax::Mouth& mouth, const Unicodes& unicodes, memory::Arena& arena, const Style style)
        : mouth(mouth), unicodes(unicodes), arena(arena), style(style) {
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
        node->category = Unicodes::Category::Ordinary;
        node->style = style;
        node->codepoint = 0;
        node->value = {};
        node->left = nullptr;
        node->right = nullptr;
        node->subscript = nullptr;
        node->superscript = nullptr;
        node->arguments = {};
        return node;
    }

    void Parser::bind(const syntax::Symbol symbol, const Node::Type type, const int weight, const bool right, const bool structural) {
        const auto index = static_cast<std::size_t>(symbol);
        if (index >= rules.size()) {
            rules.resize(index + 1, Rule{Node::Type::Variable, 0, false, false});
        }
        rules[index] = Rule{type, weight, right, structural};
    }

    Node* Parser::sequence(const char closing) {
        std::vector<Node*> items;

        while (true) {
            const syntax::Token next = lookahead();
            if (next.values.empty()) {
                break;
            }
            if (closing != 0 && next.values.size() == 1 && next.values[0] == closing) {
                advance();
                break;
            }
            if (next.values == "}" || next.values == ")") {
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
        for (std::size_t i = 0; i < items.size(); ++i) {
            slice[i] = items[i];
        }
        node->arguments = slice;
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
        const syntax::Token token = advance();
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

        auto* node = compose(Node::Type::Variable);
        node->value = token.values;
        return node;
    }

    Node* Parser::atom() {
        return script(core());
    }

    Node* Parser::group(const char closing) {
        auto* node = compose(Node::Type::Group);
        node->left = sequence(closing);
        return node;
    }

    Node* Parser::script(Node* base) {
        if (!base) return nullptr;

        Node* subscript = nullptr;
        Node* superscript = nullptr;

        while (true) {
            const syntax::Token next = lookahead();
            if (next.values == "_") {
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

        auto* node = compose(Node::Type::SubSup);
        node->left = base;
        node->subscript = subscript;
        node->superscript = superscript;
        return node;
    }

    Node* Parser::step(const int priority) {
        Node* left = atom();
        if (!left) return nullptr;

        while (true) {
            const syntax::Token next = lookahead();
            if (next.values.empty() || next.values == "}" || next.values == ")" || next.values == "]") {
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
        return sequence(0);
    }

}