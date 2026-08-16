#include "layout/grid.hpp"
#include "layout/layer.hpp"
#include <algorithm>
#include <array>

namespace layout {

    void Grid::column(const float size, const float weight) noexcept {
        columns.push_back(Track{size, weight});
    }

    void Grid::row(const float size, const float weight) noexcept {
        rows.push_back(Track{size, weight});
    }

    void Grid::gap(const float space) noexcept {
        spacing = space;
    }

    Node::Size Grid::measure(const Node::Size boundary) const noexcept {
        float width = 0.0f;
        float height = 0.0f;

        for (std::size_t index = 0; index < columns.size(); ++index) {
            width += columns[index].size;
            if (index > 0) width += spacing;
        }

        for (std::size_t index = 0; index < rows.size(); ++index) {
            height += rows[index].size;
            if (index > 0) height += spacing;
        }

        return Node::Size{
            .width = boundary.width > 0.0f ? std::min(boundary.width, width) : width,
            .height = boundary.height > 0.0f ? std::min(boundary.height, height) : height
        };
    }

    void Grid::layout(const Node::Point origin, const Node::Size size, memory::Slice<Layer*> list) const noexcept {
        if (list.empty()) return;

        const std::size_t slots = columns.empty() ? 1 : columns.size();
        const std::size_t tracks = rows.empty() ? 1 : rows.size();

        const float horizontal = spacing * static_cast<float>(slots > 0 ? slots - 1 : 0);
        const float width = size.width - horizontal;

        const float vertical = spacing * static_cast<float>(tracks > 0 ? tracks - 1 : 0);
        const float height = size.height - vertical;

        float fixed = 0.0f;
        float weight = 0.0f;
        for (const auto& item : columns) {
            if (item.weight > 0.0f) weight += item.weight;
            else fixed += item.size;
        }

        const float flex = std::max(0.0f, width - fixed);

        constexpr std::size_t capacity = 64;
        std::array<float, capacity> widths{};
        std::array<float, capacity> heights{};

        const float unit = size.width / static_cast<float>(slots);
        for (std::size_t index = 0; index < slots && index < capacity; ++index) {
            widths[index] = unit;
        }

        const float span = size.height / static_cast<float>(tracks);
        for (std::size_t index = 0; index < tracks && index < capacity; ++index) {
            heights[index] = span;
        }

        if (!columns.empty()) {
            for (std::size_t index = 0; index < columns.size() && index < capacity; ++index) {
                if (columns[index].weight > 0.0f && weight > 0.0f) {
                    widths[index] = flex * (columns[index].weight / weight);
                } else {
                    widths[index] = columns[index].size;
                }
            }
        }

        fixed = 0.0f;
        weight = 0.0f;
        for (const auto& item : rows) {
            if (item.weight > 0.0f) weight += item.weight;
            else fixed += item.size;
        }

        const float room = std::max(0.0f, height - fixed);

        if (!rows.empty()) {
            for (std::size_t index = 0; index < rows.size() && index < capacity; ++index) {
                if (rows[index].weight > 0.0f && weight > 0.0f) {
                    heights[index] = room * (rows[index].weight / weight);
                } else {
                    heights[index] = rows[index].size;
                }
            }
        }

        std::size_t index = 0;
        float y = origin.y;

        for (std::size_t row = 0; row < tracks && index < list.size(); ++row) {
            float x = origin.x;
            const float extent = row < capacity ? heights[row] : 0.0f;

            for (std::size_t column = 0; column < slots && index < list.size(); ++column) {
                const float portion = column < capacity ? widths[column] : 0.0f;

                if (Layer* layer = list[index++]) {
                    layer->layout(
                        Node::Point{x, y},
                        Node::Size{portion, extent}
                    );
                }

                x += portion + spacing;
            }

            y += extent + spacing;
        }
    }

}