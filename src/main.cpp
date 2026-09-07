#include "logger.hpp"
#include "layout/document.hpp"
#include "layout/typesetter.hpp"
#include "memory/arena.hpp"
#include "modules.hpp"
#include "render/composer.hpp"
#include "render/pdf.hpp"
#include "render/primitives/boxes.hpp"
#include "render/primitives/document.hpp"
#include "render/primitives/expression.hpp"
#include "render/primitives/fonts.hpp"
#include "render/primitives/glue.hpp"
#include "render/primitives/penalties.hpp"
#include "render/primitives/rules.hpp"
#include "syntax/cursor.hpp"
#include "syntax/expression/unicodes.hpp"
#include "syntax/lexicon.hpp"
#include "syntax/mouth.hpp"
#include "syntax/node.hpp"
#include "syntax/parser.hpp"
#include "syntax/primitives/characters.hpp"
#include "syntax/primitives/conditionals.hpp"
#include "syntax/primitives/definitions.hpp"
#include "syntax/primitives/grouping.hpp"
#include "syntax/primitives/registers.hpp"
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
    const std::filesystem::path destination = root / "build" / "main.pdf";

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

    render::typography::Shaper shaper(arena);
    render::layout::Typesetter typesetter(arena, scratch);
    render::Composer composer(arena, scratch, shaper, typesetter);

    syntax::Parser parser(mouth, arena);

    render::primitives::fonts::Selection selection;
    selection.font(font);

    // 1. REGISTER ALL PRIMITIVES FIRST
    render::primitives::document::ingest(mouth, composer.document(), state.registers());
    syntax::primitives::registers::ingest(mouth, state.registers());
    syntax::primitives::grouping::ingest(mouth);
    render::primitives::fonts::ingest(mouth, state.registers(), registry, options, scratch, selection);
    render::primitives::boxes::ingest(parser, state.registers(), shaper, typesetter, selection);
    render::primitives::glue::ingest(parser, state.registers());
    render::primitives::rules::ingest(parser, state.registers());
    render::primitives::penalties::ingest(parser, state.registers());
    render::primitives::expression::ingest(parser, unicodes);
    syntax::primitives::characters::ingest(mouth, state.registers());
    syntax::primitives::conditionals::Gate{lexicon}.ingest(mouth, state.registers());
    syntax::primitives::definitions::ingest(mouth); // Binds \def, \csname, etc.

    // 2. INGEST MASTER MODULE (main.mtex)
    if (const auto core = syntax::modules::find("main.mtex")) {
        mouth.ingest(*core);
    } else {
        std::cerr << "Error: Embedded module 'main.mtex' not found!\n";
        return 1;
    }

    // 3. READ AND INGEST USER DOCUMENT (main.tex)
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

    // 4. PARSE DOCUMENT
    const memory::Slice<syntax::Node*> outputs = parser.parse();
    const auto step = std::chrono::high_resolution_clock::now();

    for (std::size_t index = 0; index < outputs.count; ++index) {
        const syntax::Node* node = outputs[index];
        if (!node) continue;

        if (node->type == syntax::Node::Type::Expression && node->expression) {
            composer.document().append(node->expression, *font);
        } else if (node->type == syntax::Node::Type::Text || node->type == syntax::Node::Type::Paragraph) {
            if (!node->value.empty()) {
                composer.document().append(node->value, *font, 12.0f);
            }
        }
    }

    const auto pdf_start = std::chrono::high_resolution_clock::now();
    if (!render::Pdf::compose(composer, 612.0f, 792.0f, destination.string())) {
        std::cerr << "Failed to output PDF to: " << destination.string() << '\n';
        options.dispose();
        return 1;
    }
    const auto tick = std::chrono::high_resolution_clock::now();

    if (!parser.tracebacks().empty()) {
        for (const auto& trace : parser.tracebacks()) {
            std::cerr << "Traceback error: " << trace.format() << "\n";
        }
        options.dispose();
        return 1;
    }

    const auto first = std::chrono::duration<double, std::micro>(mark - time).count();
    const auto second = std::chrono::duration<double, std::micro>(step - mark).count();
    const auto fourth = std::chrono::duration<double, std::micro>(tick - pdf_start).count();
    const auto whole = std::chrono::duration<double, std::micro>(tick - start).count();

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "[Pipeline Metrics]\n";
    std::cout << "Registered font files     : " << total << '\n';
    std::cout << "Top-level AST Nodes       : " << outputs.count << '\n';
    std::cout << "Document Paragraphs       : " << composer.document().paragraphs().count << '\n';
    std::cout << "Output File               : " << destination.string() << "\n\n";

    std::cout << "[Subsystem Benchmarks]\n";
    std::cout << "Typography & Font Init    : " << first << " us\n";
    std::cout << "Pratt Syntax Parsing      : " << second << " us\n";
    std::cout << "PDF Composition & Render  : " << fourth << " us\n";
    std::cout << "Total End-to-End Execution: " << whole << " us\n";

    options.dispose();
    return 0;
}