#pragma once

#include "layout/node.hpp"
#include "layout/pager.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "typography/font.hpp"
#include "typography/shaper.hpp"

#include <string_view>
#include <vector>

namespace layout {

    class Document {
    public:
        using Page = Pager::Page;

        struct Configuration {
            float width{612.0f};
            float height{792.0f};
            float left{72.0f};
            float right{72.0f};
            float top{72.0f};
            float bottom{72.0f};
            std::int32_t columns{1};
            float gap{18.0f};
            float size{10.0f};
            float leading{12.0f};
            float indent{18.0f};
            float spacing{0.0f};
            std::int32_t align{0};
            double tolerance{2000.0};
        };

        Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper) noexcept;
        Document(memory::Arena& arena, Pager& pager, typography::Shaper& shaper, const Configuration& configuration) noexcept;

        void margin(float left, float right, float top, float bottom) noexcept;
        void paper(float width, float height) noexcept;
        void grid(std::int32_t columns, float gap) noexcept;
        void font(float size, float leading) noexcept;
        void indent(float value) noexcept;
        void spacing(float value) noexcept;
        void gap(float value) noexcept;
        void align(std::int32_t value) noexcept;
        void tolerance(double value) noexcept;
        void configure(const Configuration& configuration) noexcept;
        [[nodiscard]] const Configuration& configuration() const noexcept;

        [[nodiscard]] float measure() const noexcept;
        [[nodiscard]] const std::vector<Node*>& content() const noexcept;

        [[nodiscard]] std::vector<Node*> tokenize(const typography::Font& font, std::string_view text) const;
        void append(const typography::Font& font, std::string_view text);
        void append(Node* node);
        void append(memory::Slice<Node*> slice);
        void newline();

        [[nodiscard]] memory::Slice<Node*> compose() const;
        [[nodiscard]] memory::Slice<Page> split() const;
        void reset() noexcept;

    private:
        memory::Arena& arena;
        Pager& pager;
        typography::Shaper& shaper;
        Configuration configuration_{};
        std::vector<Node*> lines{};
        std::vector<Page> pages{};
    };

}