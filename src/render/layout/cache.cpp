#include "layout/cache.hpp"

namespace render::layout {

    bool Cache::Key::operator==(const Key& target) const noexcept {
        return font == target.font && text == target.text && size == target.size;
    }

    Cache::Cache(memory::Arena& arena, const std::size_t limit) noexcept
        : arena(arena), limit(limit) {
        slots = arena.allocate<Entry>(limit);
        for (std::size_t step = 0; step < limit; ++step) {
            slots[step].occupied = false;
        }
    }

    memory::Slice<Node*> Cache::find(const Key& key) const noexcept {
        if (limit == 0 || count == 0) return {};

        std::size_t slot = hash(key) % limit;
        for (std::size_t step = 0; step < limit; ++step) {
            const auto& item = slots[(slot + step) % limit];
            if (!item.occupied) return {};
            if (item.key == key) return item.nodes;
        }
        return {};
    }

    void Cache::insert(const Key& key, memory::Slice<Node*> nodes) noexcept {
        if (limit == 0) return;

        const std::size_t slot = hash(key) % limit;
        for (std::size_t step = 0; step < limit; ++step) {
            auto& item = slots[(slot + step) % limit];
            if (!item.occupied) {
                item.key = key;
                item.nodes = nodes;
                item.occupied = true;
                ++count;
                return;
            }
            if (item.key == key) {
                item.nodes = nodes;
                return;
            }
        }
    }

    void Cache::dispose() noexcept {
        for (std::size_t step = 0; step < limit; ++step) {
            slots[step].occupied = false;
        }
        count = 0;
    }

    std::size_t Cache::hash(const Key& key) noexcept {
        const std::size_t head = std::hash<const void*>{}(key.font);
        const std::size_t body = std::hash<std::string_view>{}(key.text);
        const std::size_t tail = std::hash<float>{}(key.size);
        return head ^ body << 1 ^ tail << 2;
    }

}