#pragma once

#include <string>
#include <map>

namespace pipeline {

    struct Type {
        std::string family;
        std::string variant;
        bool operator<(const Type& other) const {
            if (family != other.family) return family < other.family;
            return variant < other.variant;
        }
    };

    class Font {
        std::map<Type, std::string> registry;

    public:
        Font() = default;
        ~Font() = default;

        void load(const std::string& family, const std::string& variant, const std::string& path);
        [[nodiscard]] float width(const std::string& family, const std::string& variant, const std::string& text, float size) const;
        [[nodiscard]] float height(const std::string& family, const std::string& variant, float size) const;
    };

}