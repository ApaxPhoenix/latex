#pragma once

#include "font.hpp"
#include "memory/slice.hpp"

#include <cstdint>

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

        explicit Node(const Type type = Type::Box) noexcept : type_(type) {}

        [[nodiscard]] Type type() const noexcept { return type_; }
        void type(const Type value) noexcept { type_ = value; }

        [[nodiscard]] Node* next() const noexcept { return next_; }
        void next(Node* value) noexcept { next_ = value; }

        [[nodiscard]] const Box& box() const noexcept { return data.box; }
        [[nodiscard]] const Glue& glue() const noexcept { return data.glue; }
        [[nodiscard]] const Kern& kern() const noexcept { return data.kern; }
        [[nodiscard]] const Penalty& penalty() const noexcept { return data.penalty; }
        [[nodiscard]] const Rule& rule() const noexcept { return data.rule; }
        [[nodiscard]] const Glyph& glyph() const noexcept { return data.glyph; }
        [[nodiscard]] const Pause& pause() const noexcept { return data.pause; }
        [[nodiscard]] const Insertion& insertion() const noexcept { return data.insertion; }
        [[nodiscard]] const Directive& directive() const noexcept { return data.directive; }

        void box(const Box& value) noexcept { type_ = Type::Box; data.box = value; }
        void glue(const Glue& value) noexcept { type_ = Type::Glue; data.glue = value; }
        void kern(const Kern& value) noexcept { type_ = Type::Kern; data.kern = value; }
        void penalty(const Penalty& value) noexcept { type_ = Type::Penalty; data.penalty = value; }
        void rule(const Rule& value) noexcept { type_ = Type::Rule; data.rule = value; }
        void glyph(const Glyph& value) noexcept { type_ = Type::Glyph; data.glyph = value; }
        void pause(const Pause& value) noexcept { type_ = Type::Pause; data.pause = value; }
        void insertion(const Insertion& value) noexcept { type_ = Type::Insertion; data.insertion = value; }
        void directive(const Directive& value) noexcept { type_ = Type::Directive; data.directive = value; }

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
            Pause pause;
            Insertion insertion;
            Directive directive;

            Data() : box{} {}
            ~Data() {}
        } data{};
    };

}