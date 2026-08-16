#include "expression/parser.hpp"
#include "expression/unicodes.hpp"
#include "logger.hpp"
#include "memory/arena.hpp"
#include "semantics/union.hpp"
#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/parser.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

std::filesystem::path root() {
#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().parent_path();
#else
    return std::filesystem::current_path();
#endif
}

std::string load(const std::filesystem::path& path) {
#if defined(_WIN32)
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return {};

    DWORD size = GetFileSize(handle, nullptr);
    std::string source(size, '\0');
    DWORD bytes = 0;
    ReadFile(handle, source.data(), size, &bytes, nullptr);
    CloseHandle(handle);
    return source;
#else
    int descriptor = open(path.c_str(), O_RDONLY);
    if (descriptor < 0) return {};

    struct stat status{};
    fstat(descriptor, &status);
    std::string source(status.st_size, '\0');
    read(descriptor, source.data(), status.st_size);
    close(descriptor);
    return source;
#endif
}

}

int main(int count, char* args[]) {
    Logger::init(count, args);
    Logger::enable(Logger::Type::All);
    Logger::level(Logger::Level::Traceback);

    std::filesystem::path input = root() / "build" / "main.tex";

    if (count > 1) {
        input = args[1];
    }

    if (!std::filesystem::exists(input)) {
        Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Input file not found at {}", input.string());
        return 1;
    }

    memory::Arena arena(1024 * 1024);

    syntax::Lexicon lexicon(arena);
    semantics::Union state{};
    memory::Slice<syntax::Node*> nodes{};

    const std::string file_content = load(input);
    const std::string_view source = arena.copy(file_content);

    const auto start = std::chrono::high_resolution_clock::now();

    syntax::Cursor cursor(std::vector<syntax::Token>{});
    syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

    mouth.ingest(source);

    expression::Unicodes unicodes;
    syntax::Parser parser(mouth, arena);

    parser.bind("\\[", [&](const syntax::Parser& engine) -> syntax::Node* {
        expression::Parser math(engine.mouth(), unicodes, arena);
        math.parse();
        return nullptr;
    });

    parser.bind("\\texttt", [](syntax::Parser&) -> syntax::Node* {
        return nullptr;
    });

    nodes = parser.parse();

    const auto stop = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::micro> elapsed = stop - start;

    std::cout << "Mouth + Parser Time: " << elapsed.count() << " us (" << (elapsed.count() / 1000.0) << " ms)\n";
    std::cout << "Parsed Nodes Count: " << nodes.size() << "\n";

    return 0;
}