#include "logger.hpp"
#include "layout/document.hpp"
#include "layout/line.hpp"
#include "layout/pager.hpp"
#include "memory/arena.hpp"
#include "render/pdf.hpp"
#include "render/wasm.hpp"
#include "syntax/cursor.hpp"
#include "syntax/expression/parser.hpp"
#include "syntax/expression/unicodes.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/parser.hpp"
#include "syntax/semantics/union.hpp"
#include "syntax/tokens.hpp"
#include "syntax/traceback.hpp"
#include "typography/font.hpp"
#include "typography/fontconfig.hpp"
#include "typography/registry.hpp"
#include "typography/shaper.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

int main(int count, char* arguments[]) {
    Logger::init(count, arguments);
    Logger::types(Logger::Type::None);
    Logger::level(Logger::Level::Error);

    const auto start = std::chrono::high_resolution_clock::now();

    memory::Arena arena(1024 * 1024);
    memory::Arena scratch(64 * 1024);

    const auto time = std::chrono::high_resolution_clock::now();

    const std::filesystem::path binary = std::filesystem::absolute(arguments[0]);
    const std::filesystem::path root = binary.parent_path().parent_path();
    const std::filesystem::path assets = root / "assets";
    const std::filesystem::path directory = assets / "fonts";
    const std::filesystem::path configuration = directory / "aliases.conf";
    const std::filesystem::path source = root / "build" / "main.tex";
    const std::filesystem::path output = root / "build" / "main.pdf";

    if (!std::filesystem::exists(configuration)) {
        std::cerr << "Alias configuration file missing at: " << configuration.string() << '\n';
        return 1;
    }

    if (!std::filesystem::exists(source)) {
        std::cerr << "TeX file missing at: " << source.string() << '\n';
        return 1;
    }

    #if defined(_WIN32)
        _putenv_s("FONTCONFIG_FILE", configuration.string().c_str());
    #else
        setenv("FONTCONFIG_FILE", configuration.string().c_str(), 1);
    #endif

    render::typography::FontConfig options(arena);
    if (!options.compose(configuration.string())) {
        std::cerr << "Configuration load failure: " << configuration.string() << '\n';
        return 1;
    }

    std::size_t total = 0;
    if (std::filesystem::exists(directory)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                if (const auto extension = entry.path().extension().string(); extension == ".otf" || extension == ".ttf" || extension == ".pfb") {
                    if (options.compose(entry.path().string())) {
                        ++total;
                    }
                }
            }
        }
    }

    const auto location = options.find(scratch, "text");
    if (!location) {
        std::cerr << "Font query failure for family 'text'\n";
        return 1;
    }

    render::typography::Registry registry(arena);
    constexpr render::typography::Registry::Spec specification{
        .family = "text",
        .weight = 400,
        .slant = 0,
        .size = 12.0f
    };

    render::typography::Font* font = registry.get(specification, *location);
    if (!font) {
        std::cerr << "Registry font instantiation failure\n";
        return 1;
    }

    const auto mark = std::chrono::high_resolution_clock::now();

    syntax::Lexicon lexicon(arena);
    syntax::semantics::Union state{};

    syntax::expression::Unicodes unicodes;
    unicodes.compose("alpha", 0x03B1, syntax::expression::Unicodes::Category::Ordinary);
    unicodes.compose("xi", 0x03BE, syntax::expression::Unicodes::Category::Ordinary);
    unicodes.compose("pi", 0x03C0, syntax::expression::Unicodes::Category::Ordinary);
    unicodes.compose("omega", 0x03C9, syntax::expression::Unicodes::Category::Ordinary);
    unicodes.compose("infty", 0x221E, syntax::expression::Unicodes::Category::Ordinary);

    syntax::Cursor cursor(std::vector<syntax::Token>{});
    syntax::Mouth mouth(std::move(cursor), state, lexicon, arena);

    std::ifstream file(source, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open LaTeX file: " << source.string() << '\n';
        options.dispose();
        return 1;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content(static_cast<std::size_t>(size), '\0');
    if (!file.read(content.data(), size)) {
        std::cerr << "Failed to read LaTeX file: " << source.string() << '\n';
        options.dispose();
        return 1;
    }

    mouth.ingest(content);

    syntax::Parser parser(mouth, arena);

    parser.bind("\\[", [&](const syntax::Parser& instance) -> syntax::Node* {
        syntax::expression::Parser expression(
            instance.mouth(),
            unicodes,
            instance.arena(),
            syntax::expression::Node::Style::Display
        );
        syntax::expression::Node* tree = expression.parse();
        std::ignore = tree;
        return instance.arena().compose<syntax::Node>();
    });

    const memory::Slice<syntax::Node*> outputs = parser.parse();
    const auto step = std::chrono::high_resolution_clock::now();

    render::typography::Shaper shaper(arena);
    const render::typography::Font* chain[] = { font };
    constexpr std::string_view sample = "f(x) dx";

    const auto glyphs = shaper.shape(
        memory::Slice{chain, 1},
        sample,
        {}
    );

    const auto metrics = font->metrics(12.0f);
    const auto point = std::chrono::high_resolution_clock::now();

    render::layout::Document::Configuration layout{
        .width = 612.0f,
        .height = 792.0f,
        .left = 72.0f,
        .right = 72.0f,
        .top = 72.0f,
        .bottom = 72.0f,
        .leading = 14.0f
    };

    render::layout::Document document(arena, scratch, shaper, layout);
    document.append(content, *font, 12.0f);

    document.layout();
    const auto tick = std::chrono::high_resolution_clock::now();

    auto* box = render::layout::Line::horizontal(arena, glyphs, layout.width);

    render::layout::Pager::Configuration page{ .height = layout.height };
    render::layout::Pager pager(arena, page);

    render::layout::Pager::Context context{
        .height = layout.height - layout.top - layout.bottom
    };

    const auto pages = pager.paginate(box, context);
    const auto finish = std::chrono::high_resolution_clock::now();

    const auto draw = std::chrono::high_resolution_clock::now();

    memory::Slice<render::layout::Node*> nodes = arena.allocate<render::layout::Node*>(pages.count);
    for (std::size_t index = 0; index < pages.count; ++index) {
        nodes[index] = render::layout::Line::vertical(arena, pages[index].nodes, 0.0f);
    }

    render::Pdf::compose(nodes, output.string());

    const auto paper = std::chrono::high_resolution_clock::now();

    render::Wasm raster;
    raster.compose(box, static_cast<int>(layout.width), static_cast<int>(layout.height));

    const auto image = std::chrono::high_resolution_clock::now();

    if (!parser.tracebacks().empty()) {
        for (const auto& trace : parser.tracebacks()) {
            std::cerr << "Traceback error: " << trace.format() << "\n";
        }
        options.dispose();
        return 1;
    }

    const auto first = std::chrono::duration<double, std::micro>(mark - time).count();
    const auto second = std::chrono::duration<double, std::micro>(step - mark).count();
    const auto third = std::chrono::duration<double, std::micro>(point - step).count();
    const auto fourth = std::chrono::duration<double, std::micro>(tick - point).count();
    const auto fifth = std::chrono::duration<double, std::micro>(finish - tick).count();
    const auto sixth = std::chrono::duration<double, std::micro>(paper - draw).count();
    const auto seventh = std::chrono::duration<double, std::micro>(image - paper).count();
    const auto whole = std::chrono::duration<double, std::micro>(image - start).count();

    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - tick).count();

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "[Pipeline Metrics]\n";
    std::cout << "Registered font files     : " << total << '\n';
    std::cout << "Top-level AST Nodes       : " << outputs.count << '\n';
    std::cout << "Shaped Glyph Nodes        : " << glyphs.count << '\n';
    std::cout << "Document Paragraphs       : " << document.paragraphs().count << '\n';
    std::cout << "Paginated Page Count      : " << pages.count << '\n';
    std::cout << "WASM Buffer Dimensions    : " << raster.width() << "x" << raster.height() << '\n';
    std::cout << "Font Metrics Ascent/Height: " << metrics.ascent << " / " << metrics.height << "\n\n";

    std::cout << "[Subsystem Benchmarks]\n";
    std::cout << "Typography & Font Init    : " << first << " us\n";
    std::cout << "Pratt Syntax Parsing      : " << second << " us\n";
    std::cout << "HarfBuzz Glyph Shaping    : " << third << " us\n";
    std::cout << "Knuth-Plass Layout Pass   : " << fourth << " us\n";
    std::cout << "Pager Pagination Pass     : " << fifth << " us (" << nanoseconds << " ns)\n";
    std::cout << "Skia PDF Composition Pass : " << sixth << " us\n";
    std::cout << "Skia WASM Pixel Raster    : " << seventh << " us\n";
    std::cout << "Total End-to-End Execution: " << whole << " us\n";

    options.dispose();
    return 0;
}