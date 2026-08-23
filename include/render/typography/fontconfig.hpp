#pragma once

#include "memory/arena.hpp"
#include "memory/slice.hpp"

#include <optional>
#include <string_view>

#ifndef __EMSCRIPTEN__
#include <fontconfig/fontconfig.h>
#endif

namespace render::typography {

    class FontConfig {
    public:
        struct Node {
            std::string_view key{};
            std::string_view value{};
            Node* next{nullptr};
        };

        struct Entry {
            std::string_view family{};
            std::string_view path{};
            int weight{400};
            int slant{0};
        };

        explicit FontConfig(memory::Arena& arena, std::size_t slots = 256) noexcept;
        ~FontConfig() noexcept;

        FontConfig(const FontConfig&) = delete;
        FontConfig& operator=(const FontConfig&) = delete;

        FontConfig(FontConfig&&) noexcept = default;
        FontConfig& operator=(FontConfig&&) noexcept = delete;

        [[nodiscard]] bool compose(std::string_view path) const noexcept;
        void dispose() noexcept;

        [[nodiscard]] std::optional<std::string_view> find(memory::Arena& scratch, std::string_view query) const noexcept;
        [[nodiscard]] memory::Slice<Entry> list(std::string_view path = {}) const noexcept;

    private:
        memory::Arena& arena;
        #ifndef __EMSCRIPTEN__
                FcConfig* handle{nullptr};
        #else
                void* handle{nullptr};
        #endif
        std::size_t slots{0};
        Node** table{nullptr};
    };

}