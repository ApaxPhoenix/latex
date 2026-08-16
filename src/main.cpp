#include "expression/parser.hpp"
#include "expression/unicodes.hpp"
#include "layout/breaker.hpp"
#include "layout/document.hpp"
#include "layout/grid.hpp"
#include "layout/layer.hpp"
#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "logger.hpp"
#include "memory/arena.hpp"
#include "semantics/union.hpp"
#include "syntax/cursor.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/parser.hpp"
#include "typesetter/box.hpp"
#include "typesetter/equations.hpp"
#include "typesetter/paragraph.hpp"
#include "typesetter/text.hpp"
#include "typography/font.hpp"
#include "typography/fontconfig.hpp"
#include "typography/hyphenator.hpp"
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

    double parsed = 0.0;
    expression::Node* tree = nullptr;

    {
        const auto start = std::chrono::high_resolution_clock::now();

        const std::string_view source = arena.copy(load(input));

        syntax::Lexicon lexicon(arena);
        semantics::Union state{};

        syntax::Cursor cursor(std::vector<syntax::Token>{});
        syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

        mouth.ingest(source);

        expression::Unicodes unicodes;
        syntax::Parser parser(mouth, arena);

        parser.bind("\\[", [&](syntax::Parser& p) -> syntax::Node* {
            expression::Parser math(p.mouth(), unicodes, arena);
            tree = math.parse();
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

    const typography::Font* words = nullptr;
    const typography::Font* glyphs = nullptr;
    typography::Suite suite;

    {
        typography::FontConfig configuration(arena);
        const auto config = root() / "assets" / "fonts" / "aliases.conf";
        if (!configuration.append(config.string())) {
            Logger::fmt(Logger::Type::Layout, Logger::Level::Warning, "Could not load font configuration from {}", config.string());
        }

        bool text = suite.load(typography::Suite::Face::Text, configuration, "text", 12);
        bool math = suite.load(typography::Suite::Face::Equation, configuration, "equations", 12);

        if (!text) {
            const auto path = root() / "assets" / "fonts" / "text" / "lmmono12-regular.otf";
            text = suite.load(typography::Suite::Face::Text, path.string(), 12);
        }

        if (!math) {
            const auto path = root() / "assets" / "fonts" / "equation" / "NewCMMath-Regular.otf";
            math = suite.load(typography::Suite::Face::Equation, path.string(), 12);
        }

        words = suite.fetch(typography::Suite::Face::Text);
        glyphs = suite.fetch(typography::Suite::Face::Equation);

        if (!words || !words->fetch()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Failed to initialize 'text' font face");
            return 1;
        }

        if (!glyphs || !glyphs->fetch()) {
            Logger::log(Logger::Type::Layout, Logger::Level::Error, "Failed to initialize 'equations' font face");
            return 1;
        }
    }

    constexpr std::string_view phrase = "The quick brown fox jumps over the lazy dog";
    memory::Slice<layout::Node*> prose{};
    layout::Node* equation = nullptr;
    layout::Node* paragraph = nullptr;
    layout::Node* row = nullptr;
    layout::Node* column = nullptr;
    typography::Shaper shaper(arena);

    {
        typography::Hyphenator hyphenator(arena);

        layout::Breaker::Configuration rules{
            .target = 300.0,
            .leading = 14.0,
            .tolerance = 5000.0
        };
        layout::Breaker breaker(arena, rules);

        prose = typesetter::text(phrase, *words, shaper, hyphenator, arena);

        if (tree) {
            equation = typesetter::equations(*tree, *glyphs, arena);
        }

        paragraph = typesetter::paragraph(phrase, *words, shaper, hyphenator, breaker, arena);

        std::vector<layout::Node*> active;
        if (equation) active.push_back(equation);
        if (paragraph) active.push_back(paragraph);

        memory::Slice<layout::Node*> elements = arena.allocate<layout::Node*>(active.size());
        for (std::size_t index = 0; index < active.size(); ++index) {
            elements[index] = active[index];
        }

        row = typesetter::hbox(elements, arena);
        column = typesetter::vbox(elements, arena);
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

        first = arena.compose<layout::Layer>(layout::Layer::Type::Span);
        first->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

        second = arena.compose<layout::Layer>(layout::Layer::Type::Span);
        second->padding(layout::Layer::Edge{4.0f, 4.0f, 2.0f, 2.0f});

        parent->attach(first);
        parent->attach(second);

        if (!prose.empty()) {
            memory::Slice<layout::Node> flat = arena.allocate<layout::Node>(prose.size());
            for (std::size_t index = 0; index < prose.size(); ++index) {
                if (prose[index]) {
                    flat[index] = *prose[index];
                }
            }
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

    memory::Slice<layout::Page> pages{};

    {
        layout::Pager pager(arena, layout::Pager::Configuration{.target = 500.0f});
        layout::Document document(arena, pager, shaper);
        document.paper(612.0f, 792.0f);
        document.margin(54.0f, 54.0f, 54.0f, 54.0f);

        document.append(*words, phrase);
        pages = document.split();
    }

    std::cout << "Parse Time: " << parsed << " us (" << (parsed / 1000.0) << " ms)\n";
    std::cout << "Layout Time: " << duration << " us (" << (duration / 1000.0) << " ms)\n";
    std::cout << "Root Size: " << size.width << " x " << size.height << "\n";
    std::cout << "Text Stream Nodes: " << prose.size() << "\n";
    std::cout << "Has Equation Box: " << (equation != nullptr ? "Yes" : "No") << "\n";
    std::cout << "Has Paragraph Box: " << (paragraph != nullptr ? "Yes" : "No") << "\n";
    std::cout << "HBox Children: " << row->box().list.size() << "\n";
    std::cout << "VBox Children: " << column->box().list.size() << "\n";
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