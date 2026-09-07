#pragma once

#include "syntax/mouth.hpp"
#include "syntax/semantics/registers.hpp"
#include "typography/fontconfig.hpp"
#include "typography/registry.hpp"
#include "memory/arena.hpp"

namespace render::primitives::fonts {

    class Selection {
    public:
        [[nodiscard]] const typography::Font* font() const noexcept;
        void font(const typography::Font* value) noexcept;

    private:
        const typography::Font* current{nullptr};
    };

    void ingest(
        syntax::Mouth& mouth,
        syntax::semantics::Registers& registers,
        typography::Registry& registry,
        typography::FontConfig& configuration,
        memory::Arena& scratch,
        Selection& selection
    );

}