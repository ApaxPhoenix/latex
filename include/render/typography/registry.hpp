#pragma once

#include "typography/face.hpp"
#include "typography/font.hpp"
#include "memory/arena.hpp"

#include <string_view>

namespace render::typography {

    class Registry {
    public:
        struct Spec {
            std::string_view family{};
            int weight{400};
            int slant{0};
            float size{12.0f};
        };

        struct Node {
            Spec spec{};
            Face face{};
            Font font{};
            Node* next{nullptr};
        };

        explicit Registry(memory::Arena& arena, std::size_t slots = 256) noexcept;
        ~Registry() noexcept;

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        [[nodiscard]] Font* get(const Spec& spec, std::string_view path) const noexcept;

    private:
        memory::Arena& arena;
        std::size_t slots{0};
        Node** table{nullptr};
    };

}