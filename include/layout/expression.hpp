#pragma once

#include "expression/node.hpp"
#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "semantics/registers.hpp"
#include "typography/suite.hpp"

namespace layout {

    class Expression {
    public:
        Expression(memory::Arena& arena, typography::Suite& suite, semantics::Registers& registers) noexcept;

        [[nodiscard]] Node* process(const expression::Node* expression) const;

    private:
        [[nodiscard]] Node* variable(const expression::Node* expression) const;
        [[nodiscard]] Node* binary(const expression::Node* expression) const;
        [[nodiscard]] Node* fraction(const expression::Node* expression) const;
        [[nodiscard]] Node* script(const expression::Node* expression) const;
        [[nodiscard]] Node* sequence(const expression::Node* expression) const;

        memory::Arena& arena;
        typography::Suite& suite;
        semantics::Registers& registers;
    };

}