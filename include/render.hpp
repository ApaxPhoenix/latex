#pragma once

#include "pipeline/layout.hpp"
#include "pipeline/raster.hpp"

#include <string>

using namespace pipeline;

class Render {
public:
    Render() = delete;
    static void draw(const Widget* widget, const Raster& raster, const std::string& path);
};