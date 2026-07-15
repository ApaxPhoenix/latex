#pragma once

#include "pipeline/layout.hpp"

#include <string>

using namespace pipeline;

class Render {
public:
    Render() = delete;
    static void draw(const layout::Widget* widget, const raster::Raster& raster, const std::string& path);
};