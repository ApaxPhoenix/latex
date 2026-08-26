#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>

#include <cstdint>
#include <mutex>
#include <span>
#include <string_view>
#include <vector>

namespace render::typography {

    class Face {
    public:
        struct Instance {
            FT_Library library{nullptr};
            Instance() noexcept;
            ~Instance() noexcept;
        };

        inline static thread_local Instance instance{};

        Face() noexcept = default;
        ~Face() noexcept;

        Face(const Face&) = delete;
        Face& operator=(const Face&) = delete;
        Face(Face&& input) noexcept;
        Face& operator=(Face&& input) noexcept;

        [[nodiscard]] bool compose(std::string_view path) noexcept;
        [[nodiscard]] bool compose(std::span<const std::uint8_t> bytes) noexcept;
        void dispose() noexcept;

        [[nodiscard]] FT_Face ft() const noexcept { return native; }
        [[nodiscard]] hb_face_t* hb() const noexcept { return handle; }
        [[nodiscard]] std::uint32_t units() const noexcept { return scale; }
        [[nodiscard]] std::span<const std::uint8_t> data() const noexcept { return storage; }

    private:
        [[nodiscard]] bool load(std::vector<std::uint8_t> bytes) noexcept;

        FT_Face native{nullptr};
        hb_face_t* handle{nullptr};
        std::uint32_t scale{0};
        std::vector<std::uint8_t> storage{};
        mutable std::mutex mutex{};
    };

}