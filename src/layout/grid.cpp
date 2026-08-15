#include "layout/grid.hpp"
#include "layout/layer.hpp"

#include <algorithm>

namespace layout {

    void Grid::column(const float size, const float weight) noexcept {
        columns.push_back({size, weight});
    }

    void Grid::row(const float size, const float weight) noexcept {
        rows.push_back({size, weight});
    }

    void Grid::gap(const float space) noexcept {
        spacing = space;
    }

    Node::Size Grid::measure(const Node::Size /* boundary */) const noexcept {
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

        return {width, height};
    }

    void Grid::layout(const Node::Point origin, const Node::Size size, const memory::Slice<Layer*> list) const noexcept {
        if (list.empty() || columns.empty() || rows.empty()) return;

        float width = spacing * static_cast<float>(columns.size() - 1);
        float weights = 0.0f;
        for (const auto& track : columns) {
            width += track.size;
            weights += track.weight;
        }
        const float slack = std::max(0.0f, size.width - width);

        float height = spacing * static_cast<float>(rows.size() - 1);
        float flex = 0.0f;
        for (const auto& track : rows) {
            height += track.size;
            flex += track.weight;
        }
        const float extra = std::max(0.0f, size.height - height);

        std::size_t index = 0;
        float top = origin.y;

        for (const auto& row : rows) {
            const float tall = row.size + ((flex > 0.0f && row.weight > 0.0f) ? extra * row.weight / flex : 0.0f);
            float left = origin.x;

            for (const auto& column : columns) {
                if (index >= list.size()) return;
                const float span = column.size + ((weights > 0.0f && column.weight > 0.0f) ? (slack * column.weight / weights) : 0.0f);

                if (Layer* child = list[index++]) {
                    child->layout({left, top}, {span, tall});
                }
                left += span + spacing;
            }
            top += tall + spacing;
        }
    }

}