#pragma once

#include <cstdint>

#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "layout/node.hpp"

namespace layout {

    struct Context {
        float height{0.0f};
        float width{0.0f};
        float skip{0.0f};
    };

    struct Page {
        memory::Slice<Node*> nodes{};
        float height{0.0f};
        std::int32_t index{0};
        std::int32_t badness{0};
    };

    class Pager {
    public:
        struct Configuration {
            float target{0.0f};
            float height{0.0f};
            float gap{0.0f};
            std::int32_t penalty{0};
            float stretch{0.0f};
            float shrink{0.0f};
        };

        explicit Pager(memory::Arena& arena) noexcept;
        Pager(memory::Arena& arena, const Configuration& configuration) noexcept;

        void configure(const Configuration& configuration) noexcept;
        [[nodiscard]] const Configuration& configuration() const noexcept;

        [[nodiscard]] memory::Slice<Node*> split(Node* head, float target) const;
        [[nodiscard]] memory::Slice<Page> paginate(Node* head, const Context& context) const;

        [[nodiscard]] static std::int32_t badness(float actual, float target, float flex) noexcept;

    private:
        memory::Arena& arena_;
        Configuration configuration_{};
    };

}