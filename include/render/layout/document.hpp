#pragma once

#include "layout/cache.hpp"
#include "layout/paragraph.hpp"
#include "memory/arena.hpp"
#include "memory/slice.hpp"
#include "typography/shaper.hpp"

#include <string_view>

namespace render::layout {

    class Document {
    public:
        struct Configuration {
            float width{612.0f};
            float height{792.0f};
            float left{72.0f};
            float right{72.0f};
            float top{72.0f};
            float bottom{72.0f};
            float leading{14.0f};
        };

        struct Element {
            Paragraph* item{nullptr};
            Element* next{nullptr};
        };

        Document(
            memory::Arena& arena,
            memory::Arena& scratch,
            typography::Shaper& shaper
        ) noexcept;

        Document(
            memory::Arena& arena,
            memory::Arena& scratch,
            typography::Shaper& shaper,
            const Configuration& config
        ) noexcept;

        Paragraph* append(
            std::string_view text,
            const typography::Font& font,
            float size
        ) noexcept;

        void layout() noexcept;

        [[nodiscard]] memory::Slice<Paragraph*> paragraphs() const noexcept;
        [[nodiscard]] const Configuration& configuration() const noexcept;

    private:
        memory::Arena& arena;
        memory::Arena& scratch;
        typography::Shaper& shaper;
        Cache cache;
        Configuration config{};

        Element* head{nullptr};
        Element* tail{nullptr};
        std::size_t count{0};
    };

}