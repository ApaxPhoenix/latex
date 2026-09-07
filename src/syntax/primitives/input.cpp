#include "syntax/primitives/input.hpp"
#include "logger.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace syntax::primitives::input {

    namespace {

        bool confined(const std::string_view path) {
            namespace filesystem = std::filesystem;

            std::error_code error;
            const filesystem::path root = filesystem::current_path(error);
            if (error) return false;

            const filesystem::path target = filesystem::weakly_canonical(root / filesystem::path(path), error);
            if (error) return false;

            const auto inside = std::mismatch(root.begin(), root.end(), target.begin(), target.end()).first == root.end();

            if (!inside) {
                Logger::fmt(Logger::Type::Semantics, Logger::Level::Warning,
                            "Blocked \\input access outside sandbox root: {}", path);
            }
            return inside;
        }

    }

    void ingest(Mouth& mouth) {
        mouth.bind("\\input", [](Mouth& mouth) {
            const Token filename = mouth.read();
            if (!confined(filename.values)) return;

            if (const std::ifstream stream(std::string(filename.values)); stream.is_open()) {
                std::ostringstream content;
                content << stream.rdbuf();
                mouth.ingest(content.str());
            }
        });

        mouth.bind("\\immediate", [](Mouth&) {});
    }

}