#include "typography/registry.hpp"
#include "logger.hpp"

namespace typography {

    Registry::Registry(memory::Arena& arena, const std::size_t slots, const std::size_t seed) noexcept
        : arena(arena), slots(slots > 0 ? slots : 256), seed(seed) {
        auto [data, count] = arena.allocate<Node*>(this->slots);
        table = data;
        for (std::size_t index = 0; index < this->slots; ++index) table[index] = nullptr;
    }

    Registry::~Registry() noexcept {
        if (!table) return;
        for (std::size_t index = 0; index < slots; ++index) {
            Node* current = table[index];
            while (current) {
                current->font.dispose();
                current->face.dispose();
                current = current->next;
            }
        }
    }

    Font* Registry::get(const Spec& spec, const std::string_view path) const noexcept {
        if (!table) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Font registry table uninitialized");
            return nullptr;
        }

        std::size_t hash = seed;
        for (const char letter : spec.family) hash = ((hash << 5) + hash) + static_cast<std::size_t>(letter);
        hash ^= static_cast<std::size_t>(spec.weight) << 8;
        hash ^= static_cast<std::size_t>(spec.slant) << 16;
        hash ^= static_cast<std::size_t>(spec.size * 100.0f);
        const std::size_t slot = hash % slots;

        for (Node* current = table[slot]; current; current = current->next) {
            if (current->spec.family == spec.family &&
                current->spec.weight == spec.weight &&
                current->spec.slant == spec.slant &&
                current->spec.size == spec.size) {
                return &current->font;
            }
        }

        Node* node = arena.compose<Node>();
        node->spec = spec;
        node->spec.family = arena.copy(spec.family);

        if (!node->face.compose(arena.copy(path))) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed loading face for registry: {}", path);
            return nullptr;
        }

        if (!node->font.compose(node->face, spec.size)) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed loading font for registry: {}", spec.family);
            return nullptr;
        }

        node->next = table[slot];
        table[slot] = node;
        return &node->font;
    }

}