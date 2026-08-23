#include "typography/face.hpp"
#include "logger.hpp"

#include <harfbuzz/hb-ft.h>
#include <mutex>
#include <utility>

namespace render::typography {

    Face::Instance::Instance() noexcept {
        if (FT_Init_FreeType(&library) != 0) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "FreeType library init failed");
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
        std::lock_guard guard(input.mutex);
        native = std::exchange(input.native, nullptr);
        handle = std::exchange(input.handle, nullptr);
        scale = std::exchange(input.scale, 0);
        storage = std::move(input.storage);
    }

    Face& Face::operator=(Face&& input) noexcept {
        if (this != &input) {
            std::scoped_lock guard(mutex, input.mutex);
            dispose();
            native = std::exchange(input.native, nullptr);
            handle = std::exchange(input.handle, nullptr);
            scale = std::exchange(input.scale, 0);
            storage = std::move(input.storage);
        }
        return *this;
    }

    void Face::dispose() noexcept {
        std::lock_guard guard(mutex);
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
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Empty font path provided");
            return false;
        }

        if (!instance.library) return false;

        std::vector buffer(path.size() + 1, '\0');
        for (std::size_t index = 0; index < path.size(); ++index) buffer[index] = path[index];

        FT_Face loaded = nullptr;
        if (FT_New_Face(instance.library, buffer.data(), 0, &loaded) != 0) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed loading face: {}", path);
            return false;
        }

        hb_face_t* created = hb_ft_face_create_referenced(loaded);
        if (!created) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "HarfBuzz face creation failed: {}", path);
            FT_Done_Face(loaded);
            return false;
        }

        dispose();
        native = loaded;
        handle = created;
        scale = static_cast<std::uint32_t>(loaded->units_per_EM);
        return true;
    }

    bool Face::compose(const std::span<const std::uint8_t> bytes) noexcept {
        if (bytes.empty()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Empty face byte stream provided");
            return false;
        }

        if (!instance.library) return false;

        dispose();
        storage.assign(bytes.begin(), bytes.end());

        FT_Face loaded = nullptr;
        if (FT_New_Memory_Face(
                instance.library,
                storage.data(),
                static_cast<FT_Long>(storage.size()),
                0,
                &loaded) != 0) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Failed loading memory face");
            return false;
        }

        hb_face_t* created = hb_ft_face_create_referenced(loaded);
        if (!created) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "HarfBuzz memory face creation failed");
            FT_Done_Face(loaded);
            return false;
        }

        native = loaded;
        handle = created;
        scale = static_cast<std::uint32_t>(loaded->units_per_EM);
        return true;
    }

}