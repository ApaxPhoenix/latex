#include "layout/grid.hpp"
#include "layout/layer.hpp"
#include <algorithm>

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

    for (const auto& track : columns) {
        width += track.size;
    }
    if (!columns.empty()) {
        width += spacing * static_cast<float>(columns.size() - 1);
    }

    for (const auto& track : rows) {
        height += track.size;
    }
    if (!rows.empty()) {
        height += spacing * static_cast<float>(rows.size() - 1);
    }

    return Node::Size{width, height, boundary.depth};
}

void Grid::layout(const Node::Point origin, const Node::Size size, const std::vector<Layer*>& list) const noexcept {
    if (list.empty() || columns.empty() || rows.empty()) {
        return;
    }

    float base = spacing * static_cast<float>(columns.size() - 1);
    float flex = 0.0f;
    for (const auto& track : columns) {
        base += track.size;
        flex += track.weight;
    }
    const float space = std::max(0.0f, size.width - base);

    std::vector<float> widths;
    widths.reserve(columns.size());
    for (const auto& track : columns) {
        const float add = (flex > 0.0f && track.weight > 0.0f) ? (space * track.weight / flex) : 0.0f;
        widths.push_back(track.size + add);
    }

    base = spacing * static_cast<float>(rows.size() - 1);
    flex = 0.0f;
    for (const auto& track : rows) {
        base += track.size;
        flex += track.weight;
    }
    const float extra = std::max(0.0f, size.height - base);

    std::vector<float> heights;
    heights.reserve(rows.size());
    for (const auto& track : rows) {
        const float add = (flex > 0.0f && track.weight > 0.0f) ? (extra * track.weight / flex) : 0.0f;
        heights.push_back(track.size + add);
    }

    std::size_t index = 0;
    float top = origin.y;

    for (std::size_t row = 0; row < rows.size(); ++row) {
        const float tall = heights[row];
        float left = origin.x;
        for (std::size_t column = 0; column < columns.size(); ++column) {
            if (index >= list.size()) {
                return;
            }
            const float span = widths[column];
            if (Layer* child = list[index++]) {
                child->layout(Node::Point{left, top}, Node::Size{span, tall, 0.0f});
            }
            left += span + spacing;
        }
        top += tall + spacing;
    }
}

}