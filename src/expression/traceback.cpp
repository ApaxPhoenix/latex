#include "expression/traceback.hpp"

namespace expression {

    Traceback::Traceback(const Type type, const memory::Location location, const std::string_view message)
        : type_(type), location_(location), message_(message) {}

    Traceback::Type Traceback::type() const noexcept {
        return type_;
    }

    const memory::Location& Traceback::location() const noexcept {
        return location_;
    }

    std::string_view Traceback::message() const noexcept {
        return message_;
    }

}