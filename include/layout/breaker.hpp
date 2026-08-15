#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <cstdint>

namespace layout {

    class Breaker {
    public:
        struct Configuration {
            double target{0.0};
            double leading{0.0};
            double tolerance{10000.0};
            double penalty{0.0};
            double limit{0.0};
            double skip{0.0};
        };

        struct Metrics {
            double width{0.0};
            double stretch{0.0};
            double shrink{0.0};
        };

        struct Active {
            std::size_t node{0};
            std::size_t line{0};
            std::uint8_t fitness{1};
            double demerits{0.0};
            std::size_t previous{0};
        };

        Breaker(memory::Arena& arena, const Configuration& configuration) noexcept;

        static double badness(double delta, double flex) noexcept;
        static std::uint8_t classify(double ratio) noexcept;

        memory::Slice<Node*> compose(memory::Slice<Node*> input);

    private:
        memory::Arena& arena_;
        Configuration configuration_;
    };

}