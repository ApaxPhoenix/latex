#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <cstdint>

namespace render::layout {

    class Pager {
    public:
        struct Configuration {
            float height{792.0f};
        };

        struct Context {
            float height{792.0f};
        };

        struct Page {
            memory::Slice<Node*> nodes{};
            float height{0.0f};
            std::int32_t index{0};
            std::int32_t badness{0};
        };

        explicit Pager(memory::Arena& arena) noexcept;
        Pager(memory::Arena& arena, const Configuration& configuration) noexcept;

        [[nodiscard]] memory::Slice<Node*> split(const Node* head, float target) const;
        [[nodiscard]] memory::Slice<Page> paginate(const Node* head, const Context& context) const;

    private:
        memory::Arena& arena;
        Configuration configuration{};
    };

}