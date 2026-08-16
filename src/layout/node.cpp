#include "layout/node.hpp"

namespace layout {

    void Node::box(const Box& value) noexcept {
        type_ = Type::Box;
        data.box = value;
    }

    void Node::glue(const Glue& value) noexcept {
        type_ = Type::Glue;
        data.glue = value;
    }

    void Node::kern(const Kern& value) noexcept {
        type_ = Type::Kern;
        data.kern = value;
    }

    void Node::penalty(const Penalty& value) noexcept {
        type_ = Type::Penalty;
        data.penalty = value;
    }

    void Node::rule(const Rule& value) noexcept {
        type_ = Type::Rule;
        data.rule = value;
    }

    void Node::glyph(const Glyph& value) noexcept {
        type_ = Type::Glyph;
        data.glyph = value;
    }

    void Node::breaks(const Break& value) noexcept {
        type_ = Type::Break;
        data.breaks = value;
    }

    void Node::insertion(const Insertion& value) noexcept {
        type_ = Type::Insertion;
        data.insertion = value;
    }

    void Node::whatsit(const Whatsit& value) noexcept {
        type_ = Type::Whatsit;
        data.whatsit = value;
    }

}