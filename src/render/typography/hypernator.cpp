#include "typography/hyphenator.hpp"

#include <cctype>
#include <fstream>
#include <string>

namespace render::typography {

    Hyphenator::Hyphenator(memory::Arena& arena) noexcept : arena(arena) {
        root = arena.compose<Node>();
    }

    void Hyphenator::load(const std::string_view path) const {
        std::ifstream file(path.data());
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line)) {
            if (const auto pos = line.find('%'); pos != std::string::npos) {
                line.erase(pos);
            }

            std::size_t start = 0;
            while (start < line.size()) {
                while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) {
                    ++start;
                }
                if (start >= line.size()) break;

                std::size_t end = start;
                while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
                    ++end;
                }

                const std::string_view token = std::string_view(line).substr(start, end - start);
                start = end;

                std::size_t count = 0;
                for (const char character : token) {
                    if (character < '0' || character > '9') ++count;
                }
                if (count == 0) continue;

                auto characters = arena.allocate<std::uint32_t>(count);
                auto levels = arena.allocate<std::uint8_t>(count + 1);
                for (std::size_t step = 0; step <= count; ++step) levels[step] = 0;

                std::size_t index = 0;
                std::uint8_t digit = 0;
                for (const char character : token) {
                    if (character >= '0' && character <= '9') {
                        digit = static_cast<std::uint8_t>(character - '0');
                    } else {
                        levels[index] = digit;
                        characters[index] = static_cast<std::uint32_t>(static_cast<unsigned char>(character));
                        digit = 0;
                        ++index;
                    }
                }
                levels[count] = digit;

                compose(characters, levels);
            }
        }
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
        if (word.count < boundary || boundary == 0 || !root) return {};

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
            if (levels[outer] & 1) result[outer - boundary] = 1;
        }

        return result;
    }

}