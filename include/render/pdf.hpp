#pragma once

#include "layout/node.hpp"
#include "memory/slice.hpp"

#include <string_view>

namespace render {

    class Pdf {
    public:
        static bool compose(memory::Slice<layout::Node*> pages, std::string_view path);
    };

}