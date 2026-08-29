#include "syntax/primitives/input.hpp"

#include <fstream>
#include <sstream>

namespace syntax::primitives::input {

    void ingest(Mouth& mouth) {
        mouth.bind("\\input", [](Mouth& mouth) {
            if (const std::ifstream stream(std::string(mouth.read().values)); stream.is_open()) {
                std::ostringstream content;
                content << stream.rdbuf();
                mouth.ingest(content.str());
            }
        });

        mouth.bind("\\immediate", [](Mouth&) {});
    }

}