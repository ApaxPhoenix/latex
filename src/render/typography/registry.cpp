#include "typography/registry.hpp"
#include "logger.hpp"

namespace render::typography {

    Registry::Registry(memory::Arena& arena, const std::size_t slots) noexcept
        : arena(arena), slots(slots > 0 ? slots : 256) {
        auto [data, count] = arena.allocate<Node*>(this->slots);
        table = data;
        for (std::size_t index = 0; index < this->slots; ++index) table[index] = nullptr;
    }

    Registry::~Registry() noexcept {
        if (!table) return;
        for (std::size_t index = 0; index < slots; ++index) {
            Node* node = table[index];
            while (node) {
                node->font.dispose();
                node->face.dispose();
                node = node->next;
            }
        }
    }

    Font* Registry::get(const Spec& spec, const std::string_view path) noexcept {
        if (!table) {
            return nullptr;
        }

        std::size_t hash = 5381;
        for (const char letter : spec.family) hash = (hash << 5) + hash + static_cast<std::size_t>(letter);
        hash ^= static_cast<std::size_t>(spec.weight) << 8;
        hash ^= static_cast<std::size_t>(spec.slant) << 16;
        hash ^= static_cast<std::size_t>(spec.size * 100.0f);
        const std::size_t slot = hash % slots;

        for (Node* node = table[slot]; node; node = node->next) {
            if (node->spec.family == spec.family &&
                node->spec.weight == spec.weight &&
                node->spec.slant == spec.slant &&
                node->spec.size == spec.size) {
                return &node->font;
            }
        }

        Node* node = arena.compose<Node>();
        node->spec = spec;
        node->spec.family = arena.copy(spec.family);

        if (!node->face.compose(path)) {
            return nullptr;
        }

        if (!node->font.compose(node->face, spec.size)) {
            node->font.dispose();
            node->face.dispose();
            return nullptr;
        }

        node->next = table[slot];
        table[slot] = node;
        return &node->font;
    }

}