#include "layout/node.hpp"

namespace layout {

    Node::Type Node::type() const noexcept {
        return tag;
    }

    void Node::type(const Type value) noexcept {
        tag = value;
    }

    std::uint32_t Node::next() const noexcept {
        return link;
    }

    void Node::next(const std::uint32_t value) noexcept {
        link = value;
    }

    const Node::Box& Node::box() const noexcept {
        return payload.box;
    }

    Node::Box& Node::box() noexcept {
        return payload.box;
    }

    void Node::box(const Box& value) noexcept {
        tag = Type::box;
        payload.box = value;
    }

    const Node::Glue& Node::glue() const noexcept {
        return payload.glue;
    }

    Node::Glue& Node::glue() noexcept {
        return payload.glue;
    }

    void Node::glue(const Glue& value) noexcept {
        tag = Type::glue;
        payload.glue = value;
    }

    const Node::Kern& Node::kern() const noexcept {
        return payload.kern;
    }

    Node::Kern& Node::kern() noexcept {
        return payload.kern;
    }

    void Node::kern(const Kern& value) noexcept {
        tag = Type::kern;
        payload.kern = value;
    }

    const Node::Penalty& Node::penalty() const noexcept {
        return payload.penalty;
    }

    Node::Penalty& Node::penalty() noexcept {
        return payload.penalty;
    }

    void Node::penalty(const Penalty& value) noexcept {
        tag = Type::penalty;
        payload.penalty = value;
    }

    const Node::Rule& Node::rule() const noexcept {
        return payload.rule;
    }

    Node::Rule& Node::rule() noexcept {
        return payload.rule;
    }

    void Node::rule(const Rule& value) noexcept {
        tag = Type::rule;
        payload.rule = value;
    }

}