#pragma once

#include "layout/breaker.hpp"
#include "layout/cache.hpp"
#include "layout/document.hpp"
#include "layout/line.hpp"
#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "layout/paragraph.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"

namespace render::layout {

    class Typesetter {
    public:
        struct Settings {
            float baseline{12.0f};
            float limit{1.0f};
            float skip{0.0f};
        };

        Typesetter(
            memory::Arena& arena,
            memory::Arena& scratch
        ) noexcept;

        Typesetter(
            memory::Arena& arena,
            memory::Arena& scratch,
            const Settings& settings
        ) noexcept;

        [[nodiscard]] Node* stack(memory::Slice<Node*> input) const noexcept;
        [[nodiscard]] memory::Slice<Pager::Page> compose(Document& document) const;

    private:
        memory::Arena& arena;
        memory::Arena& scratch;
        Settings settings{};
    };

}