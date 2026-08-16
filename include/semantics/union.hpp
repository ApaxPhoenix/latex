#pragma once

#include "syntax/catcodes.hpp"
#include "semantics/registers.hpp"
#include "semantics/scope.hpp"

namespace semantics {

    class Union {
    public:
        void push(const Scope::Type type = Scope::Type::Group) {
            scope_.push(type);
            registers_.push();
        }

        void pop() {
            scope_.pop();
            registers_.pop();
        }

        [[nodiscard]] syntax::CatCodes& catcodes() noexcept { return catcodes_; }
        [[nodiscard]] const syntax::CatCodes& catcodes() const noexcept { return catcodes_; }

        [[nodiscard]] Registers& registers() noexcept { return registers_; }
        [[nodiscard]] const Registers& registers() const noexcept { return registers_; }

        [[nodiscard]] Scope& scope() noexcept { return scopes; }
        [[nodiscard]] const Scope& scope() const noexcept { return scopes; }

    private:
        syntax::CatCodes catcodes_{};
        Registers registers_{};
        Scope scopes{};
    };

}