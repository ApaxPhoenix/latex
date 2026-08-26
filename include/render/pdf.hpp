#pragma once

#include "render/composer.hpp"
#include <string_view>

namespace render {

    class Pdf {
    public:
        static bool compose(
            Composer& composer,
            float width,
            float height,
            std::string_view path
        );
    };

}