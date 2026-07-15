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

    auto source = R"(
        \documentclass[10pt,twocolumn]{article}

        \begin{document}

            \size{18}{\bold{\sans{Lorem Ipsum Dolor Sit Amet}}}
            \size{12}{\italic{John Doe}}

            \bold{1. Consectetur Adipiscing}
            Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \( a^2 + b^2 = c^2 \) ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.

            \[
            x_i = y_i \times z^2
            \]

            \begin{verbatim}
            Lorem ipsum dolor sit amet,
            consectetur adipiscing elit.
            \end{verbatim}

            \newpage

            \bold{2. Tempor Incididunt}
            Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit animi id est laborum.

            \[
            A = B & C \\
            D = E & F
            \]

            \row{
                \width{200}{\mono{Lorem Ipsum}}
                \width{200}{\serif{Dolor Sit}}
            }

        \end{document}
    )";

    std::cout << "Tokenizing source...\n";
    core::lexer::Lexer lexer(source);
    const std::vector<core::lexer::Token> tokens = lexer.tokenize();

    std::cout << "Parsing tokens into AST...\n";
    const core::lexer::Cursor stream(tokens);
    core::arena::Arena pool;
    core::parser::Parser parser(stream, pool);
    const std::vector<core::ast::Node*> tree = parser.parse();

    std::cout << "Loading NewCMMath-Regular font...\n";
    font::Font font;
    try {
        font.load("latin", "regular", "assets/fonts/NewCMMath-Regular.otf");
    } catch (const std::exception& error) {
        std::cerr << "Warning: Could not register font: " << error.what() << "\n";
    }

    std::cout << "Initializing raster resources...\n";
    const raster::Raster raster;

    std::cout << "Building document layout...\n";
    const layout::Layout layout(pool);

    const layout::Widget* widget = nullptr;
    if (!tree.empty()) {
        const core::ast::Node root{core::ast::Type::DOCUMENT, "root", tree};
        widget = layout.compute(&root, font, raster);
    }

    std::cout << "Rendering document to PDF...\n";
    constexpr auto path = "output.pdf";
    Render::draw(widget, raster, path);

    std::cout << "Success! Document compiled and written to: " << path << "\n";
    return 0;
}