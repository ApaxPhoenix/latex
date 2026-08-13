#pragma once

#include "memory/arena.hpp"
#include "layout/layer.hpp"

namespace layout {

    class Balance {
    public:
        Balance() = default;

        static memory::Slice<Layer> split(
            memory::Arena& arena,
            const memory::Slice<Layer> layers,
            const float limit
        ) noexcept {
            if (layers.empty() || limit <= 0.0f) {
                return {};
            }

            float total = 0.0f;
            std::size_t count = 0;

            for (const auto & layer : layers) {
                const float height = layer.size().height + layer.size().depth;
                if (total + height > limit && count > 0) {
                    break;
                }
                total += height;
                ++count;
            }

            memory::Slice<Layer> slice = arena.allocate<Layer>(count);
            for (std::size_t i = 0; i < count; ++i) {
                slice[i] = layers[i];
            }

            return slice;
        }
    };

}