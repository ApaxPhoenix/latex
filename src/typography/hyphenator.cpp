#include "typography/hyphenator.hpp"

#include <algorithm>
#include <cstring>
#include <array>

namespace typography {

    Hyphenator::Hyphenator(memory::Arena& memory) noexcept : memory(memory) {
        root = memory.compose<Node>();
    }

    void Hyphenator::insert(const std::string_view pattern) const noexcept {
        if (pattern.empty()) return;

        Node* current = root;
        std::array<std::uint8_t, 64> buffer{};
        std::size_t count = 1uz;

        for (std::size_t offset = 0uz; offset < pattern.size(); ++offset) {
            if (const char symbol = pattern[offset]; symbol >= '0' && symbol <= '9') {
                buffer[count - 1uz] = static_cast<std::uint8_t>(symbol - '0');
            } else {
                auto [iterator, inserted] = current->children.try_emplace(symbol, nullptr);
                if (inserted) {
                    iterator->second = memory.compose<Node>();
                }
                current = iterator->second;

                if (count < 64uz) {
                    buffer[count] = 0;
                    ++count;
                }
            }
        }

        current->levels.assign(buffer.begin(), buffer.begin() + count);
    }

    void Hyphenator::load(const std::string_view pattern) const noexcept {
        if (pattern.empty()) return;

        const char* pointer = pattern.data();
        const char* finish = pointer + pattern.size();

        while (pointer < finish) {
            while (pointer < finish && static_cast<unsigned char>(*pointer) <= ' ') {
                ++pointer;
            }
            if (pointer >= finish) break;

            const char* start = pointer;
            while (pointer < finish && static_cast<unsigned char>(*pointer) > ' ') {
                ++pointer;
            }

            insert(std::string_view(start, static_cast<std::size_t>(pointer - start)));
        }
    }

    std::vector<std::uint8_t> Hyphenator::analyze(const std::string_view word) const noexcept {
        if (word.size() < 3uz) return {};

        const std::size_t length = word.size();
        const std::size_t total = length + 2uz;

        std::array<char, 128> scratch{};
        char* prepared = (total <= scratch.size()) ? scratch.data() : static_cast<char*>(memory.allocate(total, alignof(char)));
        if (!prepared) return {};

        prepared[0] = '.';
        for (std::size_t outer = 0uz; outer < length; ++outer) {
            const char letter = word[outer];
            prepared[outer + 1uz] = (letter >= 'A' && letter <= 'Z') ? static_cast<char>(letter + 32) : letter;
        }
        prepared[total - 1uz] = '.';

        std::array<std::uint8_t, 128> storage{};
        std::uint8_t* levels = (total + 1uz <= storage.size()) ? storage.data() : static_cast<std::uint8_t*>(memory.allocate(total + 1uz, alignof(std::uint8_t)));
        if (!levels) return {};

        std::memset(levels, 0, (total + 1uz) * sizeof(std::uint8_t));

        for (std::size_t outer = 0uz; outer < total; ++outer) {
            const Node* current = root;
            for (std::size_t inner = outer; inner < total; ++inner) {
                const auto iterator = current->children.find(prepared[inner]);
                if (iterator == current->children.end()) break;

                current = iterator->second;

                if (const auto count = current->levels.size(); count > 0uz) {
                    const std::uint8_t* values = current->levels.data();
                    for (std::size_t offset = 0uz; offset < count; ++offset) {
                        if (const std::size_t index = outer + offset; index <= total && values[offset] > levels[index]) {
                            levels[index] = values[offset];
                        }
                    }
                }
            }
        }

        std::vector<std::uint8_t> result(length, 0);

        for (std::size_t outer = 3uz; outer < total - 2uz; ++outer) {
            if (levels[outer] & 1) {
                result[outer - 2uz] = 1;
            }
        }

        return result;
    }

}