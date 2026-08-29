#pragma once

#include "allocator.hpp"
#include "memory/arena.hpp"
#include "semantics/registers.hpp"
#include "syntax/cursor.hpp"
#include "syntax/lexer.hpp"
#include "syntax/mouth.hpp"
#include "syntax/primitives/conditionals.hpp"
#include "syntax/semantics/union.hpp"

#include <cstddef>
#include <memory>
#include <string_view>

namespace sandbox {

    struct Policy {
        bool shell{false};
        bool write{false};
        bool read{false};
        std::size_t depth{10'000};
        std::uint64_t tokens{1'000'000};
    };

    class VM {
    public:
        static constexpr std::size_t MEMORY = 64 * 1024 * 1024;

        explicit VM(const Policy& policy = {}, std::size_t limit = MEMORY);
        ~VM() = default;

        VM(const VM&) = delete;
        VM& operator=(const VM&) = delete;
        VM(VM&&) noexcept = delete;
        VM& operator=(VM&&) noexcept = delete;

        [[nodiscard]] bool eval(std::string_view code);
        [[nodiscard]] bool run(std::string_view path);

        [[nodiscard]] const Policy& rules() const noexcept { return policy; }

    private:
        void bind();

        Policy policy;
        std::unique_ptr<Allocator> memory;
        memory::Arena arena;
        syntax::semantics::Union state;
        syntax::Lexicon lexicon;
        syntax::Cursor cursor;
        syntax::semantics::Registers registers;
        syntax::primitives::conditionals::Gate gate;
        syntax::Mouth mouth;
    };

}