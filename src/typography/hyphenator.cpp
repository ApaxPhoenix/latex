#include "typography/hyphenator.hpp"

#include <algorithm>
#include <cstring>
#include <new>

namespace typography {

    Hyphenator::Hyphenator(memory::Arena& memory) noexcept : memory(memory) {
        root = memory.compose<Node>();
    }

    void Hyphenator::insert(const std::string_view pattern) const noexcept {
        if (pattern.empty()) return;

        Node* current = root;
        std::uint8_t buffer[64] = {0};
        std::size_t count = 1;

        std::size_t offset = 0;
        while (offset < pattern.size()) {
            if (const char character = pattern[offset]; character >= '0' && character <= '9') {
                buffer[count - 1] = static_cast<std::uint8_t>(character - '0');
            } else {
                if (auto next = current->children.find(character); next == current->children.end()) {
                    auto* child = memory.compose<Node>();
                    current->children[character] = child;
                    current = child;
                } else {
                    current = next->second;
                }
                if (count < 64) {
                    buffer[count] = 0;
                    count++;
                }
            }
            ++offset;
        }

        current->levels.assign(buffer, buffer + count);
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
        if (word.size() < 3) return {};

        const std::size_t length = word.size();
        const std::size_t total = length + 2;

        char scratch[128];
        char* prepared = (total <= 128) ? scratch : new (std::nothrow) char[total];
        if (!prepared) return {};

        prepared[0] = '.';
        for (std::size_t outer = 0; outer < length; ++outer) {
            const char letter = word[outer];
            prepared[outer + 1] = (letter >= 'A' && letter <= 'Z') ? static_cast<char>(letter + 32) : letter;
        }
        prepared[total - 1] = '.';

        std::uint8_t storage[128];
        std::uint8_t* levels = (total + 1 <= 128) ? storage : new (std::nothrow) std::uint8_t[total + 1];
        if (!levels) {
            if (prepared != scratch) delete[] prepared;
            return {};
        }
        std::memset(levels, 0, (total + 1) * sizeof(std::uint8_t));

        for (std::size_t outer = 0; outer < total; ++outer) {
            const Node* current = root;
            for (std::size_t inner = outer; inner < total; ++inner) {
                const auto next = current->children.find(prepared[inner]);
                if (next == current->children.end()) break;

                current = next->second;

                if (const auto count = current->levels.size(); count > 0) {
                    const std::uint8_t* values = current->levels.data();
                    for (std::size_t offset = 0; offset < count; ++offset) {
                        if (const std::size_t index = outer + offset; index <= total && values[offset] > levels[index]) {
                            levels[index] = values[offset];
                        }
                    }
                }
            }
        }

        std::vector<std::uint8_t> result(length, 0);

        for (std::size_t outer = 3; outer < total - 2; ++outer) {
            if (levels[outer] & 1) {
                result[outer - 2] = 1;
            }
        }

        if (prepared != scratch) delete[] prepared;
        if (levels != storage) delete[] levels;

        return result;
    }

}