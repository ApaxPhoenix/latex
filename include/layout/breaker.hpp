#pragma once

#include "layout/node.hpp"
#include "memory/arena.hpp"

#include <cstdint>

namespace layout {

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

        Breaker(memory::Arena& arena, const Configuration& configuration) noexcept;

        [[nodiscard]] memory::Slice<Node*> compose(memory::Slice<Node*> input) const;

    private:
        struct Metrics {
            double width{0.0};
            double stretch{0.0};
            double shrink{0.0};
        };

        struct Active {
            std::size_t node{0};
            std::size_t line{0};
            std::uint8_t fitness{0};
            double demerits{0.0};
            std::size_t previous{0};
        };

        static double badness(double delta, double flex) noexcept;
        static std::uint8_t classify(double ratio) noexcept;

        memory::Arena& arena;
        Configuration configuration;
    };

}