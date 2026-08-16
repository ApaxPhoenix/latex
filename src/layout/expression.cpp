#include "layout/expression.hpp"
#include "layout/line.hpp"

#include <algorithm>
#include <vector>

namespace layout {

    Expression::Expression(memory::Arena& arena, typography::Suite& suite, semantics::Registers& registers) noexcept
        : arena(arena), suite(suite), registers(registers) {}

    Node* Expression::process(const ::expression::Node* expression) const {
        if (!expression) return nullptr;

        switch (expression->type) {
            case ::expression::Node::Type::Variable: return variable(expression);
            case ::expression::Node::Type::Binary:   return binary(expression);
            case ::expression::Node::Type::Fraction: return fraction(expression);
            case ::expression::Node::Type::SubSup:   return script(expression);
            case ::expression::Node::Type::Sequence: return sequence(expression);
            default: return variable(expression);
        }
    }

    Node* Expression::variable(const expression::Node* expression) const {
        if (const auto* font = suite.fetch(typography::Suite::Face::Mono); !font) return nullptr;

        Node* glyph = arena.compose<Node>();
        const Node::Glyph data{
            .width = 8.0f,
            .height = 10.0f,
            .depth = 2.0f,
            .code = expression->codepoint != 0 ? expression->codepoint : static_cast<std::uint32_t>(expression->value.front())
        };
        glyph->glyph(data);
        return glyph;
    }

    Node* Expression::binary(const expression::Node* expression) const {
        std::vector<Node*> list;

        if (Node* left = process(expression->left)) list.push_back(left);

        Node* leading = arena.compose<Node>();
        leading->glue(Node::Glue{.width = 4.0f, .stretch = 1.0f, .shrink = 0.5f});
        list.push_back(leading);

        if (Node* symbol = variable(expression)) list.push_back(symbol);

        Node* trailing = arena.compose<Node>();
        trailing->glue(Node::Glue{.width = 4.0f, .stretch = 1.0f, .shrink = 0.5f});
        list.push_back(trailing);

        if (Node* right = process(expression->right)) list.push_back(right);

        memory::Slice<Node*> slice = arena.allocate<Node*>(list.size());
        std::ranges::copy(list, slice.begin());
        return Line::horizontal(arena, slice, 0.0f);
    }

    Node* Expression::fraction(const expression::Node* expression) const {
        Node* numerator = process(expression->left);
        Node* denominator = process(expression->right);

        std::vector<Node*> vertical;
        if (numerator) vertical.push_back(numerator);

        Node* rule = arena.compose<Node>();
        rule->rule(Node::Rule{.width = 20.0f, .height = 1.0f, .depth = 0.0f});
        vertical.push_back(rule);

        if (denominator) vertical.push_back(denominator);

        memory::Slice<Node*> slice = arena.allocate<Node*>(vertical.size());
        std::ranges::copy(vertical, slice.begin());
        return Line::vertical(arena, slice, 2.0f);
    }

    Node* Expression::script(const expression::Node* expression) const {
        Node* base = process(expression->left);
        Node* subscript = process(expression->subscript);
        Node* superscript = process(expression->superscript);

        std::vector<Node*> horizontal;
        if (base) horizontal.push_back(base);

        if (subscript || superscript) {
            std::vector<Node*> vertical;
            if (superscript) vertical.push_back(superscript);
            if (subscript) vertical.push_back(subscript);

            memory::Slice<Node*> slice = arena.allocate<Node*>(vertical.size());
            std::ranges::copy(vertical, slice.begin());
            horizontal.push_back(Line::vertical(arena, slice, 1.0f));
        }

        memory::Slice<Node*> slice = arena.allocate<Node*>(horizontal.size());
        std::ranges::copy(horizontal, slice.begin());
        return Line::horizontal(arena, slice, 0.0f);
    }

    Node* Expression::sequence(const expression::Node* expression) const {
        std::vector<Node*> list;
        list.reserve(expression->arguments.size());

        for (std::size_t index = 0uz; index < expression->arguments.size(); ++index) {
            if (Node* element = process(expression->arguments[index])) {
                list.push_back(element);
            }
        }

        memory::Slice<Node*> slice = arena.allocate<Node*>(list.size());
        std::ranges::copy(list, slice.begin());
        return Line::horizontal(arena, slice, 0.0f);
    }

}