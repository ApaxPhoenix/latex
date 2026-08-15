#pragma once

#include "memory/arena.hpp"

#include <fontconfig/fontconfig.h>
#include <string_view>
#include <unordered_map>
#include <vector>

typedef _FcConfig FcConfig;

namespace typography {

    struct Entry {
        std::string_view family{};
        std::string_view path{};
        int weight{0};
        int slant{0};
    };

    class FontConfig {
    public:
        explicit FontConfig(memory::Arena& arena);
        ~FontConfig();

        FontConfig(const FontConfig&) = delete;
        FontConfig& operator=(const FontConfig&) = delete;
        FontConfig(FontConfig&&) noexcept = delete;
        FontConfig& operator=(FontConfig&&) noexcept = delete;

        [[nodiscard]] bool append(std::string_view directory) const noexcept;
        [[nodiscard]] std::string_view resolve(std::string_view pattern) noexcept;
        [[nodiscard]] memory::Slice<Entry> compose(std::string_view directory) const noexcept;

    private:
        [[nodiscard]] std::string_view query(std::string_view pattern) const noexcept;

        memory::Arena& arena;
        FcConfig* configuration{};
        std::unordered_map<std::string_view, std::string_view> cache{};
    };

}