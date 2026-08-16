#pragma once

#include "semantics/union.hpp"
#include "syntax/catcodes.hpp"
#include "syntax/tokens.hpp"
#include <cstdint>

namespace semantics {

    enum class Type : std::uint8_t {
        Vertical,
        Horizontal,
        Inline,
        Display
    };

    class Governor {
    public:
        explicit Governor(Union& state) noexcept;

        void dispatch(const syntax::Token& token);
        void assign(Registers::Type type, std::size_t index, std::int32_t value, bool global = false) const;
        void set(syntax::Symbol symbol, std::int32_t value, bool global = false) const;

        [[nodiscard]] Type type() const noexcept;
        [[nodiscard]] Union& state() noexcept;
        [[nodiscard]] const Union& state() const noexcept;

    private:
        void transition(Type target);

        Union& context;
        Type type_{Type::Vertical};
    };

}