#pragma once

#include "memory/arena.hpp"
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace typography {

    class Hyphenator {
    public:
        struct Node {
            std::unordered_map<char, Node*> children;
            std::vector<std::uint8_t> levels;
        };

        explicit Hyphenator(memory::Arena& memory) noexcept;

        void load(std::string_view pattern) const noexcept;
        [[nodiscard]] std::vector<std::uint8_t> analyze(std::string_view word) const noexcept;

    private:
        void insert(std::string_view pattern) const noexcept;

        memory::Arena& memory;
        Node* root;
    };

}