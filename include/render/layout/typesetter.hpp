#pragma once

#include "layout/document.hpp"
#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "syntax/expression/node.hpp"
#include "syntax/parser.hpp"
#include "typography/font.hpp"

#include <functional>
#include <string_view>
#include <unordered_map>

namespace render::layout {

    class Typesetter {
    public:
        struct Metrics {
            float baseline{12.0f};
            float limit{1.0f};
            float skip{0.0f};
            float axis{4.0f};
            float rule{0.6f};
            float script{0.7f};
            float fraction{0.8f};
            float padding{2.0f};
        };

        using Lower = std::function<Node*(
            const syntax::expression::Node*,
            const typography::Font&,
            float,
            float,
            const Typesetter&
        )>;

        Typesetter(
            memory::Arena& arena,
            memory::Arena& scratch
        ) noexcept;

        Typesetter(
            memory::Arena& arena,
            memory::Arena& scratch,
            const Metrics& metrics
        ) noexcept;

        void bind(syntax::expression::Node::Type type, Lower handler);
        static void bind(std::string_view name, syntax::Parser::Handler handler, syntax::Parser& parser);

        [[nodiscard]] Node* lower(const syntax::expression::Node* node, const typography::Font& font, float target, float scale = 1.0f) const;
        [[nodiscard]] Node* stack(memory::Slice<Node*> input) const noexcept;
        [[nodiscard]] memory::Slice<Pager::Page> compose(Document& document) const;

        [[nodiscard]] memory::Arena& memory() const noexcept { return arena; }
        [[nodiscard]] memory::Arena& temp() const noexcept { return scratch; }
        [[nodiscard]] const Metrics& metrics() const noexcept { return config; }

    private:
        void setup();

        memory::Arena& arena;
        memory::Arena& scratch;
        Metrics config{};
        std::unordered_map<syntax::expression::Node::Type, Lower> handlers{};
    };

}