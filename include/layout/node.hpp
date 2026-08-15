#pragma once

#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <cstdint>

namespace layout {

    class alignas(16) Node {
    public:
        struct Point {
            float x{0.0f};
            float y{0.0f};
        };

        struct Size {
            float width{0.0f};
            float height{0.0f};
        };

        enum class Order : std::uint8_t {
            Normal, // Finite dimension
            Fil,    // First level infinite stretch
            Fill,   // Second level infinite stretch
            Filll   // Third level infinite stretch
        };

        enum class Type : std::uint8_t {
            Box,       // Container box (\hbox / \vbox)
            Rule,      // Geometric bar (\hrule / \vrule)
            Insertion, // Inserted content (\insert)
            Mark,      // Page header or footer mark (\mark)
            Adjust,    // Vertical adjustment (\vadjust)
            Break,     // Conditional break (\discretionary)
            Whatsit,   // Custom command extension (\special / \setfont / \documentclass)
            Glue,      // Elastic spacing (\hskip / \vskip / \glue)
            Kern,      // Fixed unbreakable space (\kern)
            Penalty,   // Line and page break cost (\penalty)
            Glyph      // Symbol character
        };

        struct Glue {
            float width{0.0f};
            float stretch{0.0f};
            float shrink{0.0f};
            Order stretchorder{Order::Normal};
            Order shrinkorder{Order::Normal};
        };

        struct Box {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
            float shift{0.0f};
            memory::Slice<Node*> list{};
        };

        struct Rule {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        struct Glyph {
            std::uint32_t font{0};
            std::uint32_t code{0};
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        struct Break {
            memory::Slice<Node*> pre{};
            memory::Slice<Node*> post{};
            memory::Slice<Node*> replace{};
        };

        struct Penalty {
            std::int32_t value{0};
        };

        struct Kern {
            float width{0.0f};
        };

        struct Insertion {
            std::uint32_t index{0};
            float height{0.0f};
        };

        struct Whatsit {
            std::uint16_t command{0};
            std::uint16_t size{0};
            void* data{nullptr};
        };

        explicit Node(const Type type) noexcept : type_(type) {}

        [[nodiscard]] Type type() const noexcept { return type_; }
        void type(const Type type) noexcept { type_ = type; }

        [[nodiscard]] Node* next() const noexcept { return link; }
        void next(Node* next_) noexcept { link = next_; }

        [[nodiscard]] const Box& box() const noexcept { return data.box; }
        Box& box() noexcept { return data.box; }
        void box(const Box& box_) noexcept;

        [[nodiscard]] const Glue& glue() const noexcept { return data.glue; }
        Glue& glue() noexcept { return data.glue; }
        void glue(const Glue& glue_) noexcept;

        [[nodiscard]] const Kern& kern() const noexcept { return data.kern; }
        Kern& kern() noexcept { return data.kern; }
        void kern(const Kern& kern_) noexcept;

        [[nodiscard]] const Penalty& penalty() const noexcept { return data.penalty; }
        Penalty& penalty() noexcept { return data.penalty; }
        void penalty(const Penalty& penalty_) noexcept;

        [[nodiscard]] const Rule& rule() const noexcept { return data.rule; }
        Rule& rule() noexcept { return data.rule; }
        void rule(const Rule& rule_) noexcept;

        [[nodiscard]] const Glyph& glyph() const noexcept { return data.glyph; }
        Glyph& glyph() noexcept { return data.glyph; }
        void glyph(const Glyph& glyph_) noexcept;

        [[nodiscard]] const Break& breaks() const noexcept { return data.breaks; }
        Break& breaks() noexcept { return data.breaks; }
        void breaks(const Break& breaks_) noexcept;

        [[nodiscard]] const Whatsit& whatsit() const noexcept { return data.whatsit; }
        Whatsit& whatsit() noexcept { return data.whatsit; }

        [[nodiscard]] const Insertion& insertion() const noexcept { return data.insertion; }
        Insertion& insertion() noexcept { return data.insertion; }

    private:
        Type type_{Type::Box};
        std::uint8_t flags{0};
        std::uint16_t reserved{0};
        Node* link{nullptr};

        union Data {
            Box box;
            Glue glue;
            Kern kern;
            Penalty penalty;
            Rule rule;
            Glyph glyph;
            Break breaks;
            Whatsit whatsit;
            Insertion insertion;

            Data() noexcept {}
            ~Data() noexcept {}
        } data;
    };

}