#pragma once

#include <cstdint>
#include <vector>

namespace semantics {

    class Scope {
    public:
        enum class Type : std::uint8_t {
            Group,       // Scope group block ({...} or \begingroup)
            Environment, // Formal environment structure (\begin...\end)
            Equations,   // Mathematical inline/display formulas ($...$, $$...$$, \(...\), \[...\])
            Box          // Layout framing box container (\hbox or \vbox)
        };

        struct Layer {
            Type type = Type::Group;
            std::size_t level = 0;
        };

        void push(Type type = Type::Group);
        void pop();
        [[nodiscard]] std::size_t depth() const noexcept;
        [[nodiscard]] Type type() const noexcept;
        void reset() noexcept;

    private:
        std::vector<Layer> stack;
    };

}