#include "layout/node.hpp"

namespace layout {

    void Node::box(const Box& box_) noexcept {
        type_ = Type::Box;
        data.box = box_;
    }

    void Node::glue(const Glue& glue_) noexcept {
        type_ = Type::Glue;
        data.glue = glue_;
    }

    void Node::kern(const Kern& kern_) noexcept {
        type_ = Type::Kern;
        data.kern = kern_;
    }

    void Node::penalty(const Penalty& penalty_) noexcept {
        type_ = Type::Penalty;
        data.penalty = penalty_;
    }

    void Node::rule(const Rule& rule_) noexcept {
        type_ = Type::Rule;
        data.rule = rule_;
    }

    void Node::glyph(const Glyph& glyph_) noexcept {
        type_ = Type::Glyph;
        data.glyph = glyph_;
    }

    void Node::breaks(const Break& breaks_) noexcept {
        type_ = Type::Break;
        data.breaks = breaks_;
    }

}