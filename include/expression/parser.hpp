#pragma once

#include "expression/node.hpp"
#include "expression/unicodes.hpp"
#include "syntax/mouth.hpp"
#include "memory/arena.hpp"

#include <vector>

namespace expression {

    struct Rule {
        Node::Type type = Node::Type::Variable;
        int weight = 0;
        bool right = false;
        bool structural = false;
    };

    class Parser {
    public:
        Parser(syntax::Mouth& mouth, const Unicodes& unicodes, memory::Arena& arena, Style style = Style::Inline);

        Node* parse();
        void bind(syntax::Symbol symbol, Node::Type type, int weight = 0, bool right = false, bool structural = false);
        [[nodiscard]] Node* compose(Node::Type type) const;

    private:
        syntax::Token advance();
        [[nodiscard]] syntax::Token lookahead() const noexcept;

        Node* sequence(char closing = 0);
        Node* step(int priority = 0);
        Node* atom();
        Node* core();
        Node* group(char closing);
        Node* script(Node* base);
        Node* structural(Node::Type type);

        syntax::Mouth& mouth;
        const Unicodes& unicodes;
        memory::Arena& arena;
        Style style = Style::Inline;
        syntax::Token current{};
        std::vector<Rule> rules{};
    };

}