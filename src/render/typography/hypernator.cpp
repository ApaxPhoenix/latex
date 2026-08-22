#include "typography/hyphenator.hpp"

namespace typography {

    Hyphenator::Hyphenator(memory::Arena& arena) noexcept : arena(arena) {
        root = arena.compose<Node>();
    }

    void Hyphenator::compose(const memory::Slice<std::uint32_t> pattern, const memory::Slice<std::uint8_t> levels) const noexcept {
        if (pattern.empty() || !root) return;

        Node* current = root;
        for (std::size_t index = 0; index < pattern.count; ++index) {
            const std::uint32_t code = pattern[index];
            Node* match = nullptr;

            for (Node* step = current->child; step; step = step->next) {
                if (step->code == code) {
                    match = step;
                    break;
                }
            }

            if (!match) {
                match = arena.compose<Node>();
                if (!match) return;
                match->code = code;
                match->next = current->child;
                current->child = match;
            }
            current = match;
        }

        current->levels = arena.allocate<std::uint8_t>(levels.count);
        for (std::size_t index = 0; index < levels.count; ++index) {
            current->levels[index] = levels[index];
        }
    }

    memory::Slice<std::uint8_t> Hyphenator::execute(
        memory::Arena& scratch,
        const memory::Slice<std::uint32_t> word,
        const std::uint32_t pad,
        const std::size_t boundary
    ) const {
        if (word.count < boundary || !root) return {};

        const std::size_t length = word.count;
        const std::size_t total = length + 2;

        auto padded = scratch.allocate<std::uint32_t>(total);
        padded[0] = pad;
        for (std::size_t index = 0; index < length; ++index) padded[index + 1] = word[index];
        padded[total - 1] = pad;

        auto levels = scratch.allocate<std::uint8_t>(total + 1);
        for (std::size_t index = 0; index <= total; ++index) levels[index] = 0;

        for (std::size_t outer = 0; outer < total; ++outer) {
            const Node* current = root;
            for (std::size_t inner = outer; inner < total && current; ++inner) {
                const std::uint32_t code = padded[inner];
                const Node* match = nullptr;

                for (const Node* step = current->child; step; step = step->next) {
                    if (step->code == code) {
                        match = step;
                        break;
                    }
                }

                current = match;
                if (current && !current->levels.empty()) {
                    for (std::size_t offset = 0; offset < current->levels.count; ++offset) {
                        if (const std::size_t target = outer + offset; target <= total && current->levels[offset] > levels[target]) {
                            levels[target] = current->levels[offset];
                        }
                    }
                }
            }
        }

        auto result = scratch.allocate<std::uint8_t>(length);
        for (std::size_t index = 0; index < length; ++index) result[index] = 0;

        for (std::size_t outer = boundary; outer < total - (boundary - 1); ++outer) {
            if (levels[outer] & 1) result[outer - 2] = 1;
        }

        return result;
    }

}