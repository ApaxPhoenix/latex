#pragma once

#include "layout/layer.hpp"

namespace layout {

    enum class Type : std::uint8_t {
        Start,
        Center,
        End,
        Stretch
    };

    class Box {
    public:
        Box() = default;

        void padding(Layer::Edge inset) noexcept;
        void margin(Layer::Edge inset) noexcept;
        void alignment(Type alignment) noexcept;

        Node::Size measure(Layer* child, Node::Size boundary) const noexcept;
        void layout(Layer* child, Node::Point origin, Node::Size size) const noexcept;

    private:
        Layer::Edge padding_{};
        Layer::Edge margin_{};
        Type alignment_{Type::Start};
    };

}