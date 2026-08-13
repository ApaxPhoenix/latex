#pragma once

#include <cstdint>

namespace layout {

    class Node {
    public:
        enum class Type : std::uint8_t {
            box,
            glue,
            kern,
            penalty,
            rule
        };

        struct Point {
            float x{0.0f};
            float y{0.0f};
        };

        struct Size {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        struct Glue {
            enum class Order : std::uint8_t {
                rigid,
                fil,
                fill,
                filll
            };

            float width{0.0f};
            float stretch{0.0f};
            float shrink{0.0f};
            Order order{Order::rigid};
        };

        struct Kern {
            float width{0.0f};
        };

        struct Penalty {
            float value{0.0f};
            bool flag{false};
        };

        struct Rule {
            float width{0.0f};
            float height{0.0f};
            float depth{0.0f};
        };

        struct Box {
            enum class Type : std::uint8_t {
                glyph,
                row,
                column,
                rule
            };

            Type type{Type::row};
            Point point{};
            Size size{};
            std::uint32_t index{0};
            std::uint32_t count{0};
        };

        Node() noexcept = default;

        [[nodiscard]] Type type() const noexcept;
        void type(Type value) noexcept;

        [[nodiscard]] std::uint32_t next() const noexcept;
        void next(std::uint32_t value) noexcept;

        [[nodiscard]] const Box& box() const noexcept;
        [[nodiscard]] Box& box() noexcept;
        void box(const Box& value) noexcept;

        [[nodiscard]] const Glue& glue() const noexcept;
        [[nodiscard]] Glue& glue() noexcept;
        void glue(const Glue& value) noexcept;

        [[nodiscard]] const Kern& kern() const noexcept;
        [[nodiscard]] Kern& kern() noexcept;
        void kern(const Kern& value) noexcept;

        [[nodiscard]] const Penalty& penalty() const noexcept;
        [[nodiscard]] Penalty& penalty() noexcept;
        void penalty(const Penalty& value) noexcept;

        [[nodiscard]] const Rule& rule() const noexcept;
        [[nodiscard]] Rule& rule() noexcept;
        void rule(const Rule& value) noexcept;

    private:
        union Storage {
            Box box;
            Glue glue;
            Kern kern;
            Penalty penalty;
            Rule rule;

            Storage() noexcept : box{} {}
        };

        Type tag{Type::box};
        std::uint32_t link{0};
        Storage payload{};
    };

}