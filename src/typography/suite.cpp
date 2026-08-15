#include "typography/suite.hpp"
#include "typography/fontconfig.hpp"

namespace typography {

    bool Suite::load(const Face face, const std::string_view path, const unsigned int size) {
        const auto index = static_cast<std::size_t>(face);
        if (index >= static_cast<std::size_t>(Face::Count)) {
            return false;
        }
        return faces[index].load(path, size);
    }

    bool Suite::load(const Face face, FontConfig& config, const std::string_view pattern, const unsigned int size) {
        const std::string_view path = config.resolve(pattern);

        if (path.empty()) {
            return load(face, pattern, size);
        }

        return load(face, path, size);
    }

    const Font* Suite::fetch(const Face face) const noexcept {
        const auto index = static_cast<std::size_t>(face);
        if (index >= static_cast<std::size_t>(Face::Count)) {
            return nullptr;
        }
        return &faces[index];
    }

    Font* Suite::fetch(const Face face) noexcept {
        const auto index = static_cast<std::size_t>(face);
        if (index >= static_cast<std::size_t>(Face::Count)) {
            return nullptr;
        }
        return &faces[index];
    }

    sk_sp<SkTypeface> Suite::typeface(const Face face) const {
        if (const auto* font = fetch(face)) {
            return font->typeface();
        }
        return nullptr;
    }

}