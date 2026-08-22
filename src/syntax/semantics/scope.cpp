#include "syntax/semantics/scope.hpp"
#include "logger.hpp"

namespace syntax::semantics {

    void Scope::push(const Type type) {
        stack.push_back(Layer{type, stack.size() + 1uz});
        Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                    "Entered new scope layer (depth: {})", stack.size());
    }

    void Scope::pop() {
        if (!stack.empty()) {
            stack.pop_back();
            Logger::fmt(Logger::Type::Semantics, Logger::Level::Debug,
                        "Exited scope layer (depth remaining: {})", stack.size());
        } else {
            Logger::log(Logger::Type::Semantics, Logger::Level::Warning,
                        "Attempted pop operation on empty scope stack");
        }
    }

    std::size_t Scope::depth() const noexcept {
        return stack.size();
    }

    Scope::Type Scope::type() const noexcept {
        if (stack.empty()) {
            using enum Type;
            return Group;
        }
        return stack.back().type;
    }

    void Scope::reset() noexcept {
        stack.clear();
        Logger::log(Logger::Type::Semantics, Logger::Level::Informative,
                    "Scope hierarchy reset to root level");
    }

}