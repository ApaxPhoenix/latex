#include "expression/parser.hpp"
#include "expression/unicodes.hpp"
#include "layout/document.hpp"
#include "layout/grid.hpp"
#include "layout/layer.hpp"
#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "layout/typesetter.hpp"
#include "logger.hpp"
#include "memory/arena.hpp"
#include "semantics/union.hpp"
#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/parser.hpp"
#include "typography/font.hpp"
#include "typography/fontconfig.hpp"
#include "typography/shaper.hpp"
#include "typography/suite.hpp"

#include <chrono>
#include <cstdlib>
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

void print(const layout::Layer& layer, int depth = 0) {
    const std::string space(depth * 2, ' ');
    const auto [horizontal, vertical] = layer.origin();
    const auto [width, height] = layer.size();

    const char* kind = "Inline";
    if (layer.type() == layout::Layer::Type::Grid) kind = "Grid";
    else if (layer.type() == layout::Layer::Type::Block) kind = "Block";

    std::cout << space << "Layer " << kind << "\n";
    std::cout << space << "Point: " << horizontal << " " << vertical << "\n";
    std::cout << space << "Extent: " << width << " " << height << "\n";

    for (const auto* child : layer.children()) {
        if (child) {
            print(*child, depth + 1);
        }
    }
}

}

int main(int count, char* args[]) {
    Logger::init(count, args);
    Logger::enable(Logger::Type::All);
    Logger::level(Logger::Level::Error);

    std::filesystem::path input = root() / "build" / "main.tex";

    if (count > 1) {
        input = args[1];
    }

    if (!std::filesystem::exists(input)) {
        Logger::fmt(Logger::Type::Layout, Logger::Level::Error, "Input file not found at {}", input.string());
        return 1;
    }

    memory::Arena arena(1024 * 1024);

    double parsed = 0.0;
    syntax::Lexicon lexicon(arena);
    semantics::Union state{};
    memory::Slice<syntax::Node*> nodes{};

    {
        const auto start = std::chrono::high_resolution_clock::now();

        const std::string_view source = arena.copy(load(input));

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
        parsed = elapsed.count();
    }

    const typography::Font* words = nullptr;
    typography::Suite suite;

    {
        const typography::Font* glyphs = nullptr;
        typography::FontConfig configuration(arena);
        if (const auto settings = root() / "assets" / "fonts" / "aliases.conf"; !configuration.append(settings.string())) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Warning, "Could not load font configuration from {}", settings.string());
        }

        bool text = suite.load(typography::Suite::Face::Regular, configuration, "text", 12);
        bool math = suite.load(typography::Suite::Face::Mono, configuration, "equations", 12);

        if (!text) {
            const auto path = root() / "assets" / "fonts" / "text" / "lmmono12-regular.otf";
            text = suite.load(typography::Suite::Face::Regular, path.string(), 12);
        }

        if (!math) {
            const auto path = root() / "assets" / "fonts" / "equation" / "NewCMMath-Regular.otf";
            math = suite.load(typography::Suite::Face::Mono, path.string(), 12);
        }

        words = suite.fetch(typography::Suite::Face::Regular);
        glyphs = suite.fetch(typography::Suite::Face::Mono);

        if (!words || !words->fetch()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Failed to initialize 'text' font face");
            return 1;
        }

        if (!glyphs || !glyphs->fetch()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Failed to initialize 'equations' font face");
            return 1;
        }
    }

    layout::Node* composition = nullptr;
    typography::Shaper shaper(arena);

    {
        const layout::Typesetter compositor(arena, suite, state, lexicon);
        composition = compositor.compose(nodes);
    }

    layout::Layer* parent = nullptr;
    layout::Layer* first = nullptr;
    layout::Layer* second = nullptr;

    {
        parent = arena.compose<layout::Layer>(layout::Layer::Type::Grid);
        parent->padding(layout::Layer::Edge{10.0f, 10.0f, 10.0f, 10.0f});
        parent->margin(layout::Layer::Edge{5.0f, 5.0f, 5.0f, 5.0f});

        auto& table = parent->grid();
        table.gap(10.0f);
        table.column(100.0f, 0.0f);
        table.column(0.0f, 1.0f);
        table.column(0.0f, 2.0f);
        table.row(40.0f, 0.0f);

        first = arena.compose<layout::Layer>(layout::Layer::Type::Inline);
        first->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

        second = arena.compose<layout::Layer>(layout::Layer::Type::Inline);
        second->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

        parent->attach(first);
        parent->attach(second);

        if (composition) {
            memory::Slice<layout::Node> flat = arena.allocate<layout::Node>(1);
            flat[0] = *composition;
            first->nodes(flat);
        }
    }

    double duration = 0.0;
    layout::Node::Size size{};

    {
        constexpr layout::Node::Size bounds{800.0f, 600.0f};
        const auto start = std::chrono::high_resolution_clock::now();

        size = parent->measure(bounds);
        parent->layout(layout::Node::Point{0.0f, 0.0f}, bounds);

        const auto stop = std::chrono::high_resolution_clock::now();

        const std::chrono::duration<double, std::micro> elapsed = stop - start;
        duration = elapsed.count();
    }

    std::size_t pages = 0;

    {
        constexpr std::string_view phrase = "The quick brown fox jumps over the lazy dog";
        layout::Pager pager(arena, layout::Pager::Configuration{.target = 500.0f});
        layout::Document document(arena, pager, shaper);
        document.paper(612.0f, 792.0f);
        document.margin(54.0f, 54.0f, 54.0f, 54.0f);

        document.append(*words, phrase);
        const auto list = document.split();
        pages = list.size();
    }

    std::cout << "Parse Time: " << parsed << " us (" << (parsed / 1000.0) << " ms)\n";
    std::cout << "Layout Time: " << duration << " us (" << (duration / 1000.0) << " ms)\n";
    std::cout << "Root Size: " << size.width << " x " << size.height << "\n";
    std::cout << "Parsed Nodes: " << nodes.size() << "\n";
    std::cout << "Has Composition Box: " << (composition != nullptr ? "Yes" : "No") << "\n";
    std::cout << "Paginated Pages: " << pages << "\n\n";

    print(*parent);

    std::cout << "\nFirst Layer (Origin & Extent): "
              << first->origin().x << ", " << first->origin().y << " | "
              << first->size().width << " x " << first->size().height << "\n";
    std::cout << "Second Layer (Origin & Extent): "
              << second->origin().x << ", " << second->origin().y << " | "
              << second->size().width << " x " << second->size().height << "\n";

    std::cout << std::flush;
    std::exit(0);
}