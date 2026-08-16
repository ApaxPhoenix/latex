#pragma once

#include <vector>
#include "layout/grid.hpp"
#include "layout/node.hpp"
#include "memory/slice.hpp"

namespace layout {

    class Layer {
    public:
        enum class Type {
            Block,
            Inline,
            Grid
        };

        struct Edge {
            float left{0.0f};
            float right{0.0f};
            float top{0.0f};
            float bottom{0.0f};
        };

        explicit Layer(Type value = Type::Block) noexcept;

        void type(Type value) noexcept;
        Type type() const noexcept;

        void nodes(memory::Slice<Node> slice) noexcept;
        memory::Slice<Node> nodes() const noexcept;

        void attach(Layer* child) noexcept;
        const std::vector<Layer*>& children() const noexcept;

        void padding(Edge space) noexcept;
        Edge padding() const noexcept;

        void margin(Edge space) noexcept;
        Edge margin() const noexcept;

        Grid& grid() noexcept;
        const Grid& grid() const noexcept;

        Node::Size measure(Node::Size boundary) noexcept;
        void layout(Node::Point origin, Node::Size size) noexcept;

        Node::Point origin() const noexcept;
        Node::Size size() const noexcept;
        Node::Size bounds() const noexcept;

    private:
        Type type_{Type::Block};
        memory::Slice<Node> elements{};
        std::vector<Layer*> list{};
        Edge padding_{};
        Edge margin_{};
        Grid table{};
        Node::Point position{};
        Node::Size extent{};
        Node::Size limit{};
    };

}