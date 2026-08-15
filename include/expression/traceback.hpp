#pragma once

#include "memory/location.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace expression {

    class Traceback {
    public:
        enum class Type : std::uint8_t {
            Syntax,
            Delimiter,
            Argument,
            Zero
        };

        Traceback() = default;
        Traceback(Type type, memory::Location location, std::string_view message);

        [[nodiscard]] Type type() const noexcept;
        [[nodiscard]] const memory::Location& location() const noexcept;
        [[nodiscard]] std::string_view message() const noexcept;

    private:
        Type type_ = Type::Syntax;
        memory::Location location_{};
        std::string message_{};
    };

}