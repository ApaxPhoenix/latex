#include "logger.hpp"
#include "layout/document.hpp"
#include "layout/line.hpp"
#include "layout/pager.hpp"
#include "memory/arena.hpp"
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
#include <iomanip>
#include <iostream>
#include <string_view>
#include <tuple>
#include <vector>

int main(int count, char* args[]) {
    Logger::init(count, args);
    Logger::types(Logger::Type::None);
    Logger::level(Logger::Level::Error);

    const auto start = std::chrono::high_resolution_clock::now();

    memory::Arena arena(1024 * 1024);
    memory::Arena scratch(64 * 1024);

    const auto time = std::chrono::high_resolution_clock::now();

    const std::filesystem::path binary = std::filesystem::absolute(args[0]);
    const std::filesystem::path root = binary.parent_path().parent_path();
    const std::filesystem::path assets = root / "assets";
    const std::filesystem::path paths = assets / "fonts";
    const std::filesystem::path config = paths / "aliases.conf";

    if (!std::filesystem::exists(config)) {
        std::cerr << "Alias configuration file missing at: " << config.string() << '\n';
        return 1;
    }

    #if defined(_WIN32)
        _putenv_s("FONTCONFIG_FILE", config.string().c_str());
    #else
        setenv("FONTCONFIG_FILE", config.string().c_str(), 1);
    #endif

    render::typography::FontConfig options(arena);
    if (!options.compose(config.string())) {
        std::cerr << "Configuration load failure: " << config.string() << '\n';
        return 1;
    }

    std::size_t total = 0;
    if (std::filesystem::exists(paths)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(paths)) {
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
    constexpr render::typography::Registry::Spec spec{
        .family = "text",
        .weight = 400,
        .slant = 0,
        .size = 12.0f
    };

    render::typography::Font* font = registry.get(spec, *location);
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

    constexpr std::string_view source = R"(\[ \hat{f}(\xi) = \int_{-\infty}^{\infty} f(x) e^{-2\pi i x \xi} \, dx \])";
    mouth.ingest(source);

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

    const auto metric = font->metrics(12.0f);
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
    document.append(sample, *font, 12.0f);
    document.append("Knuth-Plass optimal paragraph line breaking pass.", *font, 12.0f);

    document.layout();
    const auto node = std::chrono::high_resolution_clock::now();

    auto* box = render::layout::Line::horizontal(arena, glyphs, layout.width);

    render::layout::Pager::Configuration page{ .height = layout.height };
    render::layout::Pager pager(arena, page);

    render::layout::Pager::Context context{
        .height = layout.height - layout.top - layout.bottom
    };

    const auto pages = pager.paginate(box, context);
    const auto finish = std::chrono::high_resolution_clock::now();

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
    const auto fourth = std::chrono::duration<double, std::micro>(node - point).count();
    const auto fifth = std::chrono::duration<double, std::micro>(finish - node).count();
    const auto whole = std::chrono::duration<double, std::micro>(finish - start).count();

    const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - node).count();

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "[Pipeline Metrics]\n";
    std::cout << "Registered font files     : " << total << '\n';
    std::cout << "Top-level AST Nodes       : " << outputs.count << '\n';
    std::cout << "Shaped Glyph Nodes        : " << glyphs.count << '\n';
    std::cout << "Document Paragraphs       : " << document.paragraphs().count << '\n';
    std::cout << "Paginated Page Count      : " << pages.count << '\n';
    std::cout << "Font Metrics Ascent/Height: " << metric.ascent << " / " << metric.height << "\n\n";

    std::cout << "[Subsystem Benchmarks]\n";
    std::cout << "Typography & Font Init    : " << first << " us\n";
    std::cout << "Pratt Syntax Parsing      : " << second << " us\n";
    std::cout << "HarfBuzz Glyph Shaping    : " << third << " us\n";
    std::cout << "Knuth-Plass Layout Pass   : " << fourth << " us\n";
    std::cout << "Pager Pagination Pass     : " << fifth << " us (" << nanos << " ns)\n";
    std::cout << "Total End-to-End Execution: " << whole << " us\n";

    options.dispose();
    return 0;
}