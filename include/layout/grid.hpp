#pragma once

#include "layout/node.hpp"
#include "memory/slice.hpp"

#include <vector>

namespace layout {

    class Layer;

    class Grid {
    public:
        struct Track {
            float size{0.0f};
            float weight{0.0f};
        };

        void column(float size, float weight = 0.0f) noexcept;
        void row(float size, float weight = 0.0f) noexcept;
        void gap(float space) noexcept;

        [[nodiscard]] Node::Size measure(Node::Size boundary) const noexcept;
        void layout(Node::Point origin, Node::Size size, memory::Slice<Layer*> list) const noexcept;

    private:
        std::vector<Track> columns;
        std::vector<Track> rows;
        float spacing{0.0f};
    };

}