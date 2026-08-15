#include "syntax/traceback.hpp"

#include <format>
#include <utility>

namespace syntax {

    std::string Traceback::format() const {
        return std::format(
            "[{}:{}] ({}) {}",
            location_.line,
            location_.column,
            std::to_underlying(type_),
            message_
        );
    }

}