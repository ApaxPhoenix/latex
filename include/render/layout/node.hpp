#pragma once

#include "font.hpp"
#include "memory/slice.hpp"

namespace render::layout {

    class Node {
    public:
        enum class Type : std::uint8_t {
            Box,       // Container holding horizontal or vertical lists of child nodes
            Glue,      // Elastic spacing that stretches or shrinks during line or page breaking
            Kern,      // Fixed non-breakable spacing between layout elements
            Penalty,   // Breakpoint evaluation node with an attached penalty value
            Rule,      // Solid rectangular geometric shape like lines or rules
            Glyph,     // Individual rendered typographic character or symbol
            Pause,     // Explicit break directive or breakpoint container
            Insertion, // Floating content like footnotes or figures
            Directive  // Extension node for custom or specialized engine directives
        };

        enum class Order : std::uint8_t {
            Normal,    // Standard finite elasticity order
            Fil,       // First level infinite elasticity overriding normal
            Fill,      // Second level infinite elasticity overriding fil
            Filll      // Third level infinite elasticity overriding fill
        };

        enum class Alignment : std::uint8_t {
            Horizontal, // Horizontal list box alignment mode
            Vertical    // Vertical list box alignment mode
        };

        enum class Sign : std::uint8_t {
            None,       // Baseline glue state with zero scaling
            Stretching, // Glue expanded beyond baseline width
            Shrinking   // Glue compressed below baseline width
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
            float shift{0.0f};
            float ratio{0.0f};
            Sign sign{Sign::None};
            Alignment alignment{Alignment::Horizontal};
            memory::Slice<Node*> list{};
        };

        struct Glue {
            float width{0.0f};
            float stretch{0.0f};
            float shrink{0.0f};
            Order expand{Order::Normal};
            Order limit{Order::Normal};
        };

        struct Kern {
            float width{0.0f};
        };

        struct Penalty {
            std::int32_t value{0};
            bool flag{false};
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
            float x{0.0f};
            float y{0.0f};
            std::uint32_t code{0};
            const typography::Font* font{nullptr};
        };

        struct Pause {
            Penalty penalty{};
        };

        struct Insertion {
            float height{0.0f};
        };

        struct Directive {
            void* ptr{nullptr};
        };

        explicit Node(const Type type = Type::Box) noexcept
            : kind(type) {}

        [[nodiscard]] Type type() const noexcept { return kind; }
        void type(const Type value) noexcept { kind = value; }

        [[nodiscard]] Node* next() const noexcept { return link; }
        void next(Node* node) noexcept { link = node; }

        [[nodiscard]] const Box& box() const noexcept { return data.box; }
        [[nodiscard]] const Glue& glue() const noexcept { return data.glue; }
        [[nodiscard]] const Kern& kern() const noexcept { return data.kern; }
        [[nodiscard]] const Penalty& penalty() const noexcept { return data.penalty; }
        [[nodiscard]] const Rule& rule() const noexcept { return data.rule; }
        [[nodiscard]] const Glyph& glyph() const noexcept { return data.glyph; }
        [[nodiscard]] const Pause& pause() const noexcept { return data.pause; }
        [[nodiscard]] const Insertion& insertion() const noexcept { return data.insertion; }
        [[nodiscard]] const Directive& directive() const noexcept { return data.directive; }

        void box(const Box& value) noexcept { kind = Type::Box; data.box = value; }
        void glue(const Glue& value) noexcept { kind = Type::Glue; data.glue = value; }
        void kern(const Kern& value) noexcept { kind = Type::Kern; data.kern = value; }
        void penalty(const Penalty& value) noexcept { kind = Type::Penalty; data.penalty = value; }
        void rule(const Rule& value) noexcept { kind = Type::Rule; data.rule = value; }
        void glyph(const Glyph& value) noexcept { kind = Type::Glyph; data.glyph = value; }
        void pause(const Pause& value) noexcept { kind = Type::Pause; data.pause = value; }
        void insertion(const Insertion& value) noexcept { kind = Type::Insertion; data.insertion = value; }
        void directive(const Directive& value) noexcept { kind = Type::Directive; data.directive = value; }

    private:
        Type kind{Type::Box};
        Node* link{nullptr};

        union Data {
            Box box;
            Glue glue;
            Kern kern;
            Penalty penalty;
            Rule rule;
            Glyph glyph;
            Pause pause;
            Insertion insertion;
            Directive directive;

            Data() : box{} {}
            ~Data() {}
        } data{};
    };

}