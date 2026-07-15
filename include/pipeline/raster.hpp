#pragma once

#include <string>
#include <unordered_map>

namespace pipeline::raster {

    class Raster {
        std::unordered_map<std::string, void*> graphics;

    public:
        Raster() = default;
        ~Raster();

        void load(const std::string& name, const std::string& path);
        [[nodiscard]] void* look(const std::string& name) const;
    };

}