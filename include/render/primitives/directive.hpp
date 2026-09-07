#pragma once

#include "syntax/node.hpp"
#include "render/layout/node.hpp"
#include "memory/arena.hpp"
#include "memory/location.hpp"

namespace render::primitives {

    [[nodiscard]] inline syntax::Node* directive(memory::Arena& arena, layout::Node* node, const memory::Location location) noexcept {
        auto* wrapper = arena.compose<syntax::Node>(syntax::Node::Type::Directive, std::string_view{}, location);
        wrapper->directive = node;
        return wrapper;
    }

}