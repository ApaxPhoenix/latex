#include "syntax/parser.hpp"
#include "syntax/tokens.hpp"
#include "syntax/traceback.hpp"
#include "syntax/expression/parser.hpp"
#include "syntax/expression/unicodes.hpp"
#include "syntax/semantics/union.hpp"
#include "syntax/cursor.hpp"
#include "syntax/mouth.hpp"
#include "syntax/lexicon.hpp"
#include "typography/fontconfig.hpp"
#include "typography/registry.hpp"
#include "typography/shaper.hpp"
#include "typography/font.hpp"
#include "memory/arena.hpp"

#include <iostream>
#include <string_view>
#include <vector>
#include <chrono>
#include <filesystem>
#include <tuple>

int main([[maybe_unused]] int argc, char* argv[]) {
    const auto start = std::chrono::high_resolution_clock::now();

    memory::Arena arena(1024 * 1024);
    memory::Arena scratch(64 * 1024);

    const std::filesystem::path executable = std::filesystem::absolute(argv[0]);
    const std::filesystem::path root = executable.parent_path().parent_path();
    const std::filesystem::path assets = root / "assets";
    const std::filesystem::path fonts = assets / "fonts";
    const std::filesystem::path alias_config = fonts / "aliases.conf";

    if (!std::filesystem::exists(alias_config)) {
        std::cerr << "Alias configuration file missing at: " << alias_config.string() << '\n';
        return 1;
    }

    #if defined(_WIN32)
        _putenv_s("FONTCONFIG_FILE", alias_config.string().c_str());
    #else
        setenv("FONTCONFIG_FILE", alias_config.string().c_str(), 1);
    #endif

    render::typography::FontConfig configuration(arena);
    if (!configuration.compose(alias_config.string())) {
        std::cerr << "Configuration load failure: " << alias_config.string() << '\n';
        return 1;
    }

    std::size_t count = 0;
    if (std::filesystem::exists(fonts)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(fonts)) {
            if (entry.is_regular_file()) {
                const auto extension = entry.path().extension().string();
                if (extension == ".otf" || extension == ".ttf" || extension == ".pfb") {
                    if (configuration.compose(entry.path().string())) {
                        ++count;
                    }
                }
            }
        }
    }

    std::cout << "Registered font files count: " << count << '\n';

    const auto location = configuration.find(scratch, "text");
    if (!location) {
        std::cerr << "Font query failure for family 'text'\n";
        return 1;
    }

    std::cout << "Resolved text font path: " << *location << '\n';

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

    render::typography::Shaper shaper(arena);
    const render::typography::Font* fallback_chain[] = { font };
    constexpr std::string_view sample = "f(x) dx";
    
    const auto glyphs = shaper.shape(
        memory::Slice{fallback_chain, 1},
        sample,
        {}
    );

    const auto metric = font->metrics(64.0f);

    const auto finish = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();

    if (!parser.tracebacks().empty()) {
        for (const auto& trace : parser.tracebacks()) {
            std::cerr << "Traceback error: " << trace.format() << "\n";
        }
    } else {
        std::cout << "Parsed top-level AST nodes: " << outputs.count << '\n';
        std::cout << "Shaped layout nodes count: " << glyphs.count << '\n';
        std::cout << "Loaded font ascent: " << metric.ascent << ", height: " << metric.height << '\n';
        std::cout << "Total execution time: " << duration << " microseconds\n";
    }

    configuration.dispose();
    return 0;
}