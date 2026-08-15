#pragma once

#include "layout/grid.hpp"
#include "layout/node.hpp"
#include "memory/slice.hpp"

#include <vector>

namespace layout {

    class Layer {
    public:
        enum class Type : std::uint8_t {
            Block,
            Span,
            Glyph,
            Raster,
            Group,
            Grid
        };

        struct Edge {
            float left = 0.0f;
            float right = 0.0f;
            float top = 0.0f;
            float bottom = 0.0f;
        };

        explicit Layer(Type value) noexcept;

        void type(Type value) noexcept;
        [[nodiscard]] Type type() const noexcept;

        void nodes(memory::Slice<Node> slice) noexcept;
        [[nodiscard]] memory::Slice<Node> nodes() const noexcept;

        void attach(Layer* child) noexcept;
        [[nodiscard]] const std::vector<Layer*>& children() const noexcept;

        void padding(Edge space) noexcept;
        [[nodiscard]] Edge padding() const noexcept;

        void margin(Edge space) noexcept;
        [[nodiscard]] Edge margin() const noexcept;

        [[nodiscard]] Grid& grid() noexcept;
        [[nodiscard]] const Grid& grid() const noexcept;

        Node::Size measure(Node::Size boundary) noexcept;
        void layout(Node::Point origin, Node::Size size) noexcept;

        [[nodiscard]] Node::Point origin() const noexcept;
        [[nodiscard]] Node::Size size() const noexcept;
        [[nodiscard]] Node::Size bounds() const noexcept;

    private:
        Type item = Type::Block;
        Edge padding_{};
        Edge margin_{};
        Node::Point position{};
        Node::Size extent{};
        Node::Size limit{};
        memory::Slice<Node> elements{};
        std::vector<Layer*> list{};
        Grid table{};
    };

}