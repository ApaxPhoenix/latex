#include <iostream>
#include <string>
#include <vector>

#include "core/lexer.hpp"
#include "core/parser.hpp"
#include "core/arena.hpp"
#include "core/ast.hpp"

#include "pipeline/font.hpp"
#include "pipeline/layout.hpp"
#include "pipeline/raster.hpp"
#include "render.hpp"

int main() {
    std::cout << "Starting LaTeX compiling engine...\n";

    const std::string source = R"(
        This is a demo page generated from our custom LaTeX rendering pipeline.
        The font is loaded directly from our assets.
        
        \newpage
        
        This is a clean second page.
    )";

    std::cout << "Tokenizing source...\n";
    core::lexer::Lexer lexer(source);
    std::vector<core::lexer::Token> tokens = lexer.tokenize();

    std::cout << "Parsing tokens into AST...\n";
    core::lexer::Cursor stream(tokens);
    core::arena::Arena pool;
    core::parser::Parser parser(stream, pool);
    std::vector<core::ast::Node*> tree = parser.parse();

    std::cout << "Loading NewCMMath-Regular font...\n";
    Font font;
    try {
        font.load("latin", "regular", "assets/fonts/NewCMMath-Regular.otf");
    } catch (const std::exception& error) {
        std::cerr << "Warning: Could not register font: " << error.what() << "\n";
    }

    std::cout << "Initializing raster resources...\n";
    Raster raster;

    std::cout << "Building document layout...\n";
    Layout layout(pool);

    const Widget* widget = nullptr;
    if (!tree.empty()) {
        core::ast::Node root{core::ast::Type::DOCUMENT, "root", tree};
        widget = layout.compute(&root, font, raster);
    }

    std::cout << "Rendering document to PDF...\n";
    constexpr auto path = "output.pdf";
    Render::draw(widget, raster, path);

    std::cout << "Success! Document compiled and written to: " << path << "\n";
    return 0;
}