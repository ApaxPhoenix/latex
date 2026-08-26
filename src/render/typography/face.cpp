#include "typography/face.hpp"
#include "logger.hpp"

#include <harfbuzz/hb-ft.h>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>

namespace render::typography {

    thread_local Face::Instance Face::instance{};

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

        const std::string name(path);
        std::ifstream file(name, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed opening font file: {}", path);
            return false;
        }

        const std::streamsize length = file.tellg();
        if (length <= 0) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Empty font file: {}", path);
            return false;
        }

        file.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        if (!file.read(reinterpret_cast<char*>(bytes.data()), length)) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed reading font file: {}", path);
            return false;
        }

        if (!load(std::move(bytes))) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Failed loading face: {}", path);
            return false;
        }
        return true;
    }

    bool Face::compose(const std::span<const std::uint8_t> bytes) noexcept {
        if (bytes.empty()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Empty face byte stream provided");
            return false;
        }

        return load(std::vector(bytes.begin(), bytes.end()));
    }

    bool Face::load(std::vector<std::uint8_t> bytes) noexcept {
        if (!instance.library) return false;

        FT_Face loaded = nullptr;
        if (FT_New_Memory_Face(
                instance.library,
                bytes.data(),
                static_cast<FT_Long>(bytes.size()),
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

        std::lock_guard guard(mutex);

        if (handle) {
            hb_face_destroy(handle);
        }
        if (native) {
            FT_Done_Face(native);
        }

        storage = std::move(bytes);
        native = loaded;
        handle = created;
        scale = static_cast<std::uint32_t>(loaded->units_per_EM);
        return true;
    }

}