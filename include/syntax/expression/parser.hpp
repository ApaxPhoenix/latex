#pragma once

#include "syntax/expression/node.hpp"
#include "syntax/expression/unicodes.hpp"
#include "syntax/mouth.hpp"
#include "memory/arena.hpp"

#include <cstddef>
#include <vector>

namespace syntax::expression {

    class Parser {
    public:
        struct Rule {
            Node::Type type = Node::Type::Variable;
            int weight = 0;
            bool right = false;
            bool structural = false;
        };

        Parser(Mouth& mouth, const Unicodes& unicodes, memory::Arena& arena, Node::Style style = Node::Style::Inline);

        Node* parse();
        Node* parse(char closing);
        Node* parse(Symbol closing);

        void bind(Symbol symbol, Node::Type type, int weight = 0, bool right = false, bool structural = false);
        [[nodiscard]] Node* compose(Node::Type type) const;
        [[nodiscard]] Token pending() const noexcept;

    private:
        Token advance();
        [[nodiscard]] Token lookahead() const noexcept;

        Node* sequence(char closing = 0, Symbol stop = kInvalidSymbol);
        Node* step(int priority = 0);
        Node* atom();
        Node* core();
        Node* group(char closing);
        Node* script(Node* base);
        Node* structural(Node::Type type);
        Node* command(const Token& token);

        Mouth& mouth;
        const Unicodes& unicodes;
        memory::Arena& arena;
        Node::Style style = Node::Style::Inline;
        Token current{};
        std::vector<Rule> rules{};
        std::size_t depth = 0;
        std::size_t limit = 256;
    };

}