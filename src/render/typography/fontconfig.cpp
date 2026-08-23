#include "typography/fontconfig.hpp"
#include "logger.hpp"

#ifndef __EMSCRIPTEN__
#include <cstring>
#include <filesystem>
#endif

namespace render::typography {

#ifndef __EMSCRIPTEN__

    namespace {
        const char* string(memory::Arena& scratch, const std::string_view view) noexcept {
            auto [data, count] = scratch.allocate<char>(view.size() + 1);
            std::memcpy(data, view.data(), view.size());
            data[view.size()] = '\0';
            return data;
        }
    }

    FontConfig::FontConfig(memory::Arena& arena, const std::size_t slots) noexcept
        : arena(arena), handle(FcConfigCreate()), slots(slots > 0 ? slots : 256) {
        auto [data, count] = arena.allocate<Node*>(this->slots);
        table = data;
        for (std::size_t index = 0; index < this->slots; ++index) table[index] = nullptr;
        if (handle) {
            FcConfigSetCurrent(handle);
        }
    }

    FontConfig::~FontConfig() noexcept {
        dispose();
    }

    void FontConfig::dispose() noexcept {
        if (handle) {
            FcConfigDestroy(handle);
            handle = nullptr;
        }
    }

    bool FontConfig::compose(const std::string_view path) const noexcept {
        if (!handle || path.empty()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Invalid font config path");
            return false;
        }

        const auto* buffer = reinterpret_cast<const FcChar8*>(string(arena, path));
        const std::filesystem::path location(path);

        if (std::filesystem::is_directory(location)) {
            if (FcConfigAppFontAddDir(handle, buffer)) {
                FcConfigSetCurrent(handle);
                return true;
            }
            return false;
        }

        if (const auto extension = location.extension().string(); extension == ".otf" || extension == ".ttf" || extension == ".pfb" || extension == ".ttc") {
            if (FcConfigAppFontAddFile(handle, buffer)) {
                FcConfigSetCurrent(handle);
                return true;
            }
            return false;
        }

        if (FcConfigParseAndLoad(handle, buffer, FcTrue)) {
            FcConfigSetCurrent(handle);
            return true;
        }

        Logger::fmt(Logger::Type::Layout, Logger::Level::Warning, "Font config load failed: {}", path);
        return false;
    }

    std::optional<std::string_view> FontConfig::find(memory::Arena& scratch, const std::string_view query) const noexcept {
        if (query.empty() || !table) return std::nullopt;

        std::size_t hash = 5381;
        for (const char letter : query) hash = ((hash << 5) + hash) + static_cast<std::size_t>(letter);
        const std::size_t slot = hash % slots;

        for (const Node* current = table[slot]; current; current = current->next) {
            if (current->key == query) return current->value;
        }

        if (!handle) return std::nullopt;

        const auto* buffer = reinterpret_cast<const FcChar8*>(string(scratch, query));
        FcPattern* target = FcNameParse(buffer);
        if (!target) return std::nullopt;

        FcConfigSubstitute(handle, target, FcMatchPattern);
        FcDefaultSubstitute(target);

        FcResult result;
        FcPattern* match = FcFontMatch(handle, target, &result);
        FcPatternDestroy(target);
        if (!match) return std::nullopt;

        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
            const auto* resource = reinterpret_cast<const char*>(file);
            const std::string_view view = arena.copy(std::string_view(resource, std::strlen(resource)));
            FcPatternDestroy(match);

            Node* node = arena.compose<Node>();
            node->key = arena.copy(query);
            node->value = view;
            node->next = table[slot];
            table[slot] = node;

            return node->value;
        }

        FcPatternDestroy(match);
        return std::nullopt;
    }

    memory::Slice<FontConfig::Entry> FontConfig::list(const std::string_view path) const noexcept {
        if (!handle) return {};

        const FcFontSet* set = FcConfigGetFonts(handle, FcSetApplication);
        if (!set) return {};

        std::size_t count = 0;
        for (int index = 0; index < set->nfont; ++index) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(set->fonts[index], FC_FILE, 0, &file) == FcResultMatch && file) {
                if (const std::string_view location(reinterpret_cast<const char*>(file)); path.empty() || location.find(path) != std::string_view::npos) {
                    ++count;
                }
            }
        }

        if (count == 0) return {};

        auto slice = arena.allocate<Entry>(count);
        std::size_t offset = 0;

        for (int index = 0; index < set->nfont && offset < count; ++index) {
            const FcPattern* font = set->fonts[index];
            FcChar8* file = nullptr;
            FcChar8* family = nullptr;
            int weight = 0, slant = 0;

            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch && file) {
                std::string_view location = arena.copy(reinterpret_cast<const char*>(file));
                if (!path.empty() && location.find(path) == std::string_view::npos) continue;

                std::string_view name{};
                if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && family) {
                    name = arena.copy(reinterpret_cast<const char*>(family));
                }

                FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
                FcPatternGetInteger(font, FC_SLANT, 0, &slant);

                slice[offset++] = Entry{.family = name, .path = location, .weight = weight, .slant = slant};
            }
        }

        return slice;
    }

#else

    FontConfig::FontConfig(memory::Arena& arena, const std::size_t slots) noexcept
        : arena(arena), handle(nullptr), slots(slots > 0 ? slots : 256) {
        auto [data, count] = arena.allocate<Node*>(this->slots);
        table = data;
        for (std::size_t index = 0; index < this->slots; ++index) table[index] = nullptr;
    }

    FontConfig::~FontConfig() noexcept {
        dispose();
    }

    void FontConfig::dispose() noexcept {}

    bool FontConfig::compose(const std::string_view path) const noexcept {
        if (path.empty() || !table) return false;

        std::size_t position = path.find_last_of("/\\");
        std::string_view filename = (position == std::string_view::npos) ? path : path.substr(position + 1);
        std::size_t boundary = filename.find_last_of('.');
        std::string_view family = (boundary == std::string_view::npos) ? filename : filename.substr(0, boundary);

        std::size_t hash = 5381;
        for (const char letter : family) hash = ((hash << 5) + hash) + static_cast<std::size_t>(letter);
        const std::size_t slot = hash % slots;

        for (const Node* current = table[slot]; current; current = current->next) {
            if (current->key == family) return true;
        }

        Node* node = arena.compose<Node>();
        node->key = arena.copy(family);
        node->value = arena.copy(path);
        node->next = table[slot];
        table[slot] = node;

        return true;
    }

    std::optional<std::string_view> FontConfig::find(memory::Arena& scratch, const std::string_view query) const noexcept {
        if (query.empty() || !table) return std::nullopt;

        std::size_t hash = 5381;
        for (const char letter : query) hash = ((hash << 5) + hash) + static_cast<std::size_t>(letter);
        const std::size_t slot = hash % slots;

        for (const Node* current = table[slot]; current; current = current->next) {
            if (current->key == query) return current->value;
        }

        for (std::size_t index = 0; index < slots; ++index) {
            for (const Node* current = table[index]; current; current = current->next) {
                if (current->key.find(query) != std::string_view::npos || query.find(current->key) != std::string_view::npos) {
                    return current->value;
                }
            }
        }

        return std::nullopt;
    }

    memory::Slice<FontConfig::Entry> FontConfig::list(const std::string_view path) const noexcept {
        if (!table) return {};

        std::size_t count = 0;
        for (std::size_t index = 0; index < slots; ++index) {
            for (const Node* current = table[index]; current; current = current->next) {
                if (path.empty() || current->value.find(path) != std::string_view::npos) {
                    ++count;
                }
            }
        }

        if (count == 0) return {};

        auto slice = arena.allocate<Entry>(count);
        std::size_t offset = 0;

        for (std::size_t index = 0; index < slots && offset < count; ++index) {
            for (const Node* current = table[index]; current; current = current->next) {
                if (path.empty() || current->value.find(path) != std::string_view::npos) {
                    slice[offset++] = Entry{
                        .family = current->key,
                        .path = current->value,
                        .weight = 400,
                        .slant = 0
                    };
                }
            }
        }

        return slice;
    }

#endif

}