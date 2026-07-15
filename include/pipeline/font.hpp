#pragma once

#include <string>
#include <unordered_map>
#include <cairo.h>

namespace pipeline::font {

    struct Key {
        size_t id;
        bool operator==(const Key& other) const { return id == other.id; }
    };

    struct Hash {
        size_t operator()(const Key& key) const { return key.id; }
    };

    class Font {
    public:
        Font();
        ~Font();

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        void load(const std::string& family, const std::string& variant, const std::string& path);
        [[nodiscard]] float width(const std::string& family, const std::string& variant, const std::string& text, float size) const;
        [[nodiscard]] float height(const std::string& family, const std::string& variant, float size) const;

    private:
        std::unordered_map<Key, cairo_font_face_t*, Hash> faces;
        cairo_surface_t* surface = nullptr;
        cairo_t* context = nullptr;

        static size_t hash(const std::string& family, const std::string& variant) {
            const size_t h1 = std::hash<std::string>{}(family);
            const size_t h2 = std::hash<std::string>{}(variant);
            return h1 ^ h2 << 1;
        }
    };

}