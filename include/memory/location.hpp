#pragma once

#include <cstddef>
#include <cstdint>

namespace memory {

    struct Location {
        std::uint32_t line{1};
        std::uint32_t column{1};
        std::size_t offset{0};
    };

}