#include "typography/face.hpp"
#include "logger.hpp"

#include <harfbuzz/hb-ft.h>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>
#include <span>

namespace render::typography {

    thread_local Face::Instance Face::core{};

    Face::Instance::Instance() noexcept {
        if (FT_Init_FreeType(&library) != 0) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Error");
        }
    }

    Face::Instance::~Instance() noexcept {
        if (library) {
            FT_Done_FreeType(library);
        }
    }

    Face::~Face() noexcept {
        dispose();
    }

    Face::Face(Face&& input) noexcept {
        std::lock_guard lock(input.mutex);
        native = std::exchange(input.native, nullptr);
        handle = std::exchange(input.handle, nullptr);
        scale = std::exchange(input.scale, 0);
        storage = std::move(input.storage);
    }

    Face& Face::operator=(Face&& input) noexcept {
        if (this != &input) {
            std::scoped_lock lock(mutex, input.mutex);

            if (handle) {
                hb_face_destroy(handle);
            }
            if (native) {
                FT_Done_Face(native);
            }

            native = std::exchange(input.native, nullptr);
            handle = std::exchange(input.handle, nullptr);
            scale = std::exchange(input.scale, 0);
            storage = std::move(input.storage);
        }
        return *this;
    }

    void Face::dispose() noexcept {
        std::lock_guard lock(mutex);

        if (handle) {
            hb_face_destroy(handle);
            handle = nullptr;
        }
        if (native) {
            FT_Done_Face(native);
            native = nullptr;
        }
        scale = 0;
        storage.clear();
        storage.shrink_to_fit();
    }

    bool Face::compose(const std::string_view path) noexcept {
        if (path.empty()) {
            return false;
        }

        const std::string name(path);
        std::ifstream file(name, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        const std::streamsize size = file.tellg();
        if (size <= 0) {
            return false;
        }

        file.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
        if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
            return false;
        }

        return load(std::move(data));
    }

    bool Face::compose(const std::span<const std::uint8_t> data) noexcept {
        if (data.empty()) {
            return false;
        }

        return load(std::vector(data.begin(), data.end()));
    }

    bool Face::load(std::vector<std::uint8_t> data) noexcept {
        if (!core.library) return false;

        FT_Face face = nullptr;
        if (FT_New_Memory_Face(
                core.library,
                data.data(),
                static_cast<FT_Long>(data.size()),
                0,
                &face) != 0) {
            return false;
        }

        hb_face_t* font = hb_ft_face_create_referenced(face);
        if (!font) {
            FT_Done_Face(face);
            return false;
        }

        std::lock_guard lock(mutex);

        if (handle) {
            hb_face_destroy(handle);
        }
        if (native) {
            FT_Done_Face(native);
        }

        storage = std::move(data);
        native = face;
        handle = font;
        scale = static_cast<std::uint32_t>(face->units_per_EM);
        return true;
    }

}