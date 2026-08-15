#include "expression/parser.hpp"
#include "layout/breaker.hpp"
#include "layout/document.hpp"
#include "layout/grid.hpp"
#include "layout/layer.hpp"
#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "memory/arena.hpp"
#include "semantics/union.hpp"
#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/parser.hpp"
#include "syntax/unicodes.hpp"
#include "typography/font.hpp"
#include "typography/fontconfig.hpp"
#include "typography/shaper.hpp"
#include "typography/suite.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
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
    const auto [x, y] = layer.origin();
    const auto [width, height] = layer.size();

    std::cout << space << "Layer " << (layer.type() == layout::Layer::Type::Grid ? "Grid" : "Span") << "\n";
    std::cout << space << "Point: " << x << " " << y << "\n";
    std::cout << space << "Extent: " << width << " " << height << "\n";

    for (const auto* child : layer.children()) {
        if (child) {
            print(*child, depth + 1);
        }
    }
}

}

int main(int count, char* args[]) {
    std::filesystem::path input = root() / "build" / "main.tex";

    if (count > 1) {
        input = args[1];
    }

    if (!std::filesystem::exists(input)) {
        std::cerr << "Error: Input file not found at " << input << "\n";
        return 1;
    }

    memory::Arena arena(1024 * 1024);

    double parsed = 0.0;
    expression::Node* ast = nullptr;

    {
        const auto start = std::chrono::high_resolution_clock::now();

        const std::string_view source = arena.copy(load(input));

        syntax::Lexicon lexicon(arena);
        semantics::Union state{};

        syntax::Cursor cursor(std::vector<syntax::Token>{});
        syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

        mouth.ingest(source);

        syntax::Unicodes unicodes;
        syntax::Parser parser(mouth, arena);

        parser.bind("\\[", [&](syntax::Parser& p) -> syntax::Node* {
            expression::Parser math(p.mouth(), unicodes, arena);
            ast = math.parse();
            return nullptr;
        });

        parser.bind("\\texttt", [](syntax::Parser&) -> syntax::Node* {
            return nullptr;
        });

        memory::Slice<syntax::Node*> nodes = parser.parse();
        std::ignore = nodes;

        const auto stop = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double, std::micro> elapsed = stop - start;
        parsed = elapsed.count();
    }

    typography::FontConfig configuration(arena);
    const auto aliases = root() / "assets" / "fonts" / "aliases.conf";
    std::ignore = configuration.append(aliases.string());

    typography::Suite suite;
    suite.load(typography::Suite::Face::Text, configuration, "letters", 12);
    suite.load(typography::Suite::Face::Equation, configuration, "glyphs", 12);

    const auto* words = suite.fetch(typography::Suite::Face::Text);
    const auto* glyphs = suite.fetch(typography::Suite::Face::Equation);

    typography::Font fallback{};
    if (!words) {
        words = &fallback;
    }
    if (!glyphs) {
        glyphs = &fallback;
    }

    auto* parent = arena.compose<layout::Layer>(layout::Layer::Type::Grid);
    parent->padding(layout::Layer::Edge{10.0f, 10.0f, 10.0f, 10.0f});
    parent->margin(layout::Layer::Edge{5.0f, 5.0f, 5.0f, 5.0f});

    auto& table = parent->grid();
    table.gap(10.0f);
    table.column(100.0f, 0.0f);
    table.column(0.0f, 1.0f);
    table.column(0.0f, 2.0f);
    table.row(40.0f, 0.0f);

    auto* first = arena.compose<layout::Layer>(layout::Layer::Type::Span);
    first->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

    auto* second = arena.compose<layout::Layer>(layout::Layer::Type::Span);
    second->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

    parent->attach(first);
    parent->attach(second);

    typography::Shaper shaper(arena);
    constexpr std::string_view phrase = "The quick brown fox jumps over the lazy dog";
    constexpr std::string_view formula = "P(x) = 3x^3 - 7x^2 + 4x - 12";

    memory::Slice<layout::Node*> prose = shaper.shape(*words, phrase);
    memory::Slice<layout::Node*> symbols = shaper.shape(*glyphs, formula);

    if (!prose.empty()) {
        memory::Slice<layout::Node> flat = arena.allocate<layout::Node>(prose.size());
        for (std::size_t index = 0; index < prose.size(); ++index) {
            if (prose[index]) {
                flat[index] = *prose[index];
            }
        }
        first->nodes(flat);
    }

    if (!symbols.empty()) {
        memory::Slice<layout::Node> flat = arena.allocate<layout::Node>(symbols.size());
        for (std::size_t index = 0; index < symbols.size(); ++index) {
            if (symbols[index]) {
                flat[index] = *symbols[index];
            }
        }
        second->nodes(flat);
    }

    double duration = 0.0;
    layout::Node::Size size{};
    {
        constexpr layout::Node::Size bounds{800.0f, 600.0f};
        const auto start = std::chrono::high_resolution_clock::now();

        size = parent->measure(bounds);
        parent->layout(layout::Node::Point{0.0f, 0.0f}, bounds);

        const auto stop = std::chrono::high_resolution_clock::now();
        std::ignore = size;

        const std::chrono::duration<double, std::micro> elapsed = stop - start;
        duration = elapsed.count();
    }

    layout::Breaker::Configuration rules{
        .target = 300.0,
        .leading = 14.0,
        .tolerance = 5000.0
    };
    layout::Breaker breaker(arena, rules);
    memory::Slice<layout::Node*> lines = breaker.compose(prose);

    layout::Pager pager(arena, layout::Pager::Configuration{.target = 500.0f});
    layout::Document document(arena, pager, shaper);
    document.paper(612.0f, 792.0f);
    document.margin(54.0f, 54.0f, 54.0f, 54.0f);

    if (!lines.empty()) {
        document.append(*words, phrase);
        document.append(*glyphs, formula);
    }
    memory::Slice<layout::Page> pages = document.split();

    std::cout << "Parse Time: " << parsed << " us (" << (parsed / 1000.0) << " ms)\n";
    std::cout << "Layout Time: " << duration << " us (" << (duration / 1000.0) << " ms)\n";
    std::cout << "Root Size: " << size.width << " x " << size.height << "\n";
    std::cout << "Letter Nodes Count: " << prose.size() << "\n";
    std::cout << "Polynomial Nodes Count: " << symbols.size() << "\n";
    std::cout << "Line-Broken Boxes: " << lines.size() << "\n";
    std::cout << "Paginated Pages: " << pages.size() << "\n\n";

    print(*parent);

    std::cout << "\nFirst Layer (Origin & Extent): "
              << first->origin().x << ", " << first->origin().y << " | "
              << first->size().width << " x " << first->size().height << "\n";
    std::cout << "Second Layer (Origin & Extent): "
              << second->origin().x << ", " << second->origin().y << " | "
              << second->size().width << " x " << second->size().height << "\n";

    return 0;
}