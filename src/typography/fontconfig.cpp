#include "typography/fontconfig.hpp"

#include <fontconfig/fontconfig.h>

#include <cstring>
#include <utility>

namespace typography {

    FontConfig::FontConfig(memory::Arena& arena) : arena(arena), configuration(FcConfigCreate()) {
        if (configuration) {
            FcConfigSetCurrent(configuration);
        }
    }

    FontConfig::~FontConfig() {
        if (configuration) {
            FcConfigDestroy(configuration);
        }
    }

    bool FontConfig::append(const std::string_view directory) const noexcept {
        if (!configuration || directory.empty()) {
            return false;
        }

        auto* buffer = static_cast<char*>(arena.allocate(directory.size() + 1, alignof(char)));
        std::memcpy(buffer, directory.data(), directory.size());
        buffer[directory.size()] = '\0';

        const auto path = reinterpret_cast<const FcChar8*>(buffer);
        if (FcConfigParseAndLoad(configuration, path, FcTrue)) {
            return true;
        }
        if (FcConfigAppFontAddDir(configuration, path)) {
            return true;
        }
        return FcConfigAppFontAddFile(configuration, path);
    }

    std::string_view FontConfig::resolve(const std::string_view pattern) noexcept {
        if (pattern.empty()) {
            return {};
        }

        if (const auto iterator = cache.find(pattern); iterator != cache.end()) {
            return iterator->second;
        }

        const std::string_view path = query(pattern);
        if (path.empty()) {
            return {};
        }

        const std::string_view key = arena.copy(pattern);
        return cache.emplace(key, path).first->second;
    }

    std::string_view FontConfig::query(const std::string_view pattern) const noexcept {
        if (!configuration || pattern.empty()) {
            return {};
        }

        auto* buffer = static_cast<char*>(arena.allocate(pattern.size() + 1, alignof(char)));
        std::memcpy(buffer, pattern.data(), pattern.size());
        buffer[pattern.size()] = '\0';

        const auto name = reinterpret_cast<const FcChar8*>(buffer);
        FcPattern* target = FcNameParse(name);
        if (!target) {
            return {};
        }

        FcConfigSubstitute(configuration, target, FcMatchPattern);
        FcDefaultSubstitute(target);

        FcResult result;
        FcPattern* match = FcFontMatch(configuration, target, &result);
        FcPatternDestroy(target);

        if (!match) {
            return {};
        }

        FcChar8* file = nullptr;
        if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
            const auto* spot = reinterpret_cast<const char*>(file);
            const std::string_view view = arena.copy(std::string_view(spot, std::strlen(spot)));
            FcPatternDestroy(match);
            return view;
        }

        FcPatternDestroy(match);
        return {};
    }

    memory::Slice<Entry> FontConfig::compose(const std::string_view directory) const noexcept {
        if (!configuration) {
            return {};
        }

        const FcFontSet* set = FcConfigGetFonts(configuration, FcSetApplication);
        if (!set) {
            return {};
        }

        std::vector<Entry> list;
        for (int index = 0; index < set->nfont; ++index) {
            const FcPattern* font = set->fonts[index];
            FcChar8* file = nullptr;
            FcChar8* family = nullptr;
            int weight = 0;
            int slant = 0;

            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch && file) {
                std::string_view location = arena.copy(reinterpret_cast<const char*>(file));

                if (!directory.empty() && location.find(directory) == std::string_view::npos) {
                    continue;
                }

                std::string_view name{};
                if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && family) {
                    name = arena.copy(reinterpret_cast<const char*>(family));
                }

                FcPatternGetInteger(font, FC_WEIGHT, 0, &weight);
                FcPatternGetInteger(font, FC_SLANT, 0, &slant);

                list.push_back(Entry{name, location, weight, slant});
            }
        }

        memory::Slice<Entry> slice = arena.allocate<Entry>(list.size());
        std::ranges::copy(list, slice.begin());
        return slice;
    }

}