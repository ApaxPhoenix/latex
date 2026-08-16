#pragma once

#include "memory/slice.hpp"

#include <cstdint>

namespace layout {

    class Node {
    public:
        enum class Type : std::uint8_t {
            Box,       // Container holding horizontal or vertical lists of child nodes
            Glue,      // Elastic spacing that stretches or shrinks during line/page breaking
            Kern,      // Fixed, non-breakable spacing between nodes
            Penalty,   // Breakpoint evaluation node with an attached penalty value
            Rule,      // Solid rectangular geometric shape (e.g., horizontal/vertical lines)
            Glyph,     // Individual rendered typographic character/symbol
            Break,     // Explicit break directive or breakpoint container
            Insertion, // Floating or inserted content (e.g., footnotes, figures)
            Whatsit    // Extension node for custom/specialized engine directives
        };

        enum class Order : std::uint8_t {
            Normal,    // Standard finite elasticity order
            Fil,       // 1st-level infinite elasticity (overrides Normal)
            Fill,      // 2nd-level infinite elasticity (overrides Fil)
            Filll      // 3rd-level infinite elasticity (overrides Fill)
        };

        struct Point {
            float x{0.0f};
            float y{0.0f};
        };

        struct Size {
            float width{0.0f};
            float height{0.0f};
        };

        struct Box {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
            memory::Slice<Node*> list{};
        };

        struct Glue {
            float width{0.0f};
            float stretch{0.0f};
            float shrink{0.0f};
            Order stretchorder{Order::Normal};
            Order shrinkorder{Order::Normal};
        };

        struct Kern {
            float width{0.0f};
        };

        struct Penalty {
            std::int32_t value{0};
        };

        struct Rule {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        struct Glyph {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
            std::uint32_t code{0};
        };

        struct Break {
            Penalty penalty{};
        };

        struct Insertion {
            float height{0.0f};
        };

        struct Whatsit {
            void* ptr{nullptr};
        };

        Node() noexcept = default;

        [[nodiscard]] Type type() const noexcept { return type_; }
        [[nodiscard]] Node* next() const noexcept { return next_; }
        void next(Node* node) noexcept { next_ = node; }

        [[nodiscard]] const Box& box() const noexcept { return data.box; }
        [[nodiscard]] const Glue& glue() const noexcept { return data.glue; }
        [[nodiscard]] const Kern& kern() const noexcept { return data.kern; }
        [[nodiscard]] const Penalty& penalty() const noexcept { return data.penalty; }
        [[nodiscard]] const Rule& rule() const noexcept { return data.rule; }
        [[nodiscard]] const Glyph& glyph() const noexcept { return data.glyph; }
        [[nodiscard]] const Break& breaks() const noexcept { return data.breaks; }
        [[nodiscard]] const Insertion& insertion() const noexcept { return data.insertion; }
        [[nodiscard]] const Whatsit& whatsit() const noexcept { return data.whatsit; }

        void box(const Box& value) noexcept;
        void glue(const Glue& value) noexcept;
        void kern(const Kern& value) noexcept;
        void penalty(const Penalty& value) noexcept;
        void rule(const Rule& value) noexcept;
        void glyph(const Glyph& value) noexcept;
        void breaks(const Break& value) noexcept;
        void insertion(const Insertion& value) noexcept;
        void whatsit(const Whatsit& value) noexcept;

    private:
        Type type_{Type::Box};
        Node* next_{nullptr};

        union Data {
            Box box;
            Glue glue;
            Kern kern;
            Penalty penalty;
            Rule rule;
            Glyph glyph;
            Break breaks;
            Insertion insertion;
            Whatsit whatsit;

            Data() {}
            ~Data() {}
        } data;
    };

}