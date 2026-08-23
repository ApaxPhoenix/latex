#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

namespace render::layout {

    class Breaker {
    public:
        struct Configuration {
            double target{400.0};
            double leading{14.0};
            double tolerance{2000.0};
            double penalty{0.0};
            double skip{12.0};
            double limit{0.0};
        };

        Breaker(memory::Arena& arena, memory::Arena& scratch, const Configuration& configuration) noexcept;

        [[nodiscard]] memory::Slice<Node*> compose(memory::Slice<Node*> input) const;

    private:
        memory::Arena& arena;
        memory::Arena& scratch;
        Configuration configuration{};
    };

}