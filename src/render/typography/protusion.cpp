#include "typography/protrusion.hpp"

namespace typography {

    Protrusion::Protrusion(memory::Arena& arena, const std::size_t slots, const std::uint32_t seed) noexcept
        : arena(arena), slots(slots), seed(seed) {
        if (slots > 0) {
            auto [data, count] = arena.allocate<Node*>(slots);
            table = data;
            for (std::size_t index = 0; index < slots; ++index) table[index] = nullptr;
        }
    }

    void Protrusion::compose(const std::uint32_t code, const std::int32_t left, const std::int32_t right) const noexcept {
        if (!table || slots == 0) return;

        const std::size_t slot = (code * seed) % slots;
        for (Node* current = table[slot]; current; current = current->next) {
            if (current->code == code) {
                current->margin = {left, right};
                return;
            }
        }

        Node* node = arena.compose<Node>();
        node->code = code;
        node->margin = {left, right};
        node->next = table[slot];
        table[slot] = node;
    }

    Protrusion::Margin Protrusion::get(const std::uint32_t code) const noexcept {
        if (!table || slots == 0) return {};

        const std::size_t slot = (code * seed) % slots;
        for (const Node* current = table[slot]; current; current = current->next) {
            if (current->code == code) return current->margin;
        }
        return {};
    }

}