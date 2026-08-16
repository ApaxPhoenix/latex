#include "semantics/governor.hpp"

namespace semantics {

    Governor::Governor(Union& state) noexcept
        : context(state) {}

    Type Governor::type() const noexcept {
        return type_;
    }

    Union& Governor::state() noexcept {
        return context;
    }

    const Union& Governor::state() const noexcept {
        return context;
    }

    void Governor::transition(const Type target) {
        if (type_ == target) return;

        switch (target) {
            case Type::Inline:
            case Type::Display:
                context.push(Scope::Type::Equations);
                break;
            case Type::Horizontal:
            case Type::Vertical:
                if (context.scope().type() == Scope::Type::Equations) {
                    context.pop();
                }
                break;
        }
        type_ = target;
    }

    void Governor::assign(const Registers::Type type, const std::size_t index, const std::int32_t value, const bool global) const {
        context.registers().assign(type, index, value, global);
    }

    void Governor::set(const syntax::Symbol symbol, const std::int32_t value, const bool global) const {
        context.registers().set(symbol, value, global);
    }

    void Governor::dispatch(const syntax::Token& token) {
        switch (token.type) {
            case syntax::CatCodes::Type::Group:
                if (!token.value.empty() && token.value.front() == '}') {
                    context.pop();
                } else {
                    context.push(Scope::Type::Group);
                }
                break;

            case syntax::CatCodes::Type::Shift:
                if (type_ == Type::Inline || type_ == Type::Display) {
                    transition(Type::Horizontal);
                } else {
                    transition(Type::Inline);
                }
                break;

            case syntax::CatCodes::Type::Letter:
            case syntax::CatCodes::Type::Other:
                if (type_ == Type::Vertical) {
                    transition(Type::Horizontal);
                }
                break;

            default:
                break;
        }
    }

}