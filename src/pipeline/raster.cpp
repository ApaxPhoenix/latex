#include "pipeline/raster.hpp"

#include <cairo.h>

namespace pipeline {

    Raster::~Raster() {
        for (auto& [name, surface] : this->graphics) {
            if (surface) {
                cairo_surface_destroy(static_cast<cairo_surface_t*>(surface));
            }
        }
        this->graphics.clear();
    }

    void Raster::load(const std::string& name, const std::string& path) {
        if (cairo_surface_t* surface = cairo_image_surface_create_from_png(path.c_str()); cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
            this->graphics[name] = surface;
        }
    }

    void* Raster::look(const std::string& name) const {
        const auto lookup = this->graphics.find(name);
        if (lookup == this->graphics.end()) {
            return nullptr;
        }
        return lookup->second;
    }

}