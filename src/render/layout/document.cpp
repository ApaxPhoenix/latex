#include "layout/document.hpp"

namespace render::layout {

    Document::Document(
        memory::Arena& arena,
        memory::Arena& scratch,
        typography::Shaper& shaper
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), cache(arena, 2048) {}

    Document::Document(
        memory::Arena& arena,
        memory::Arena& scratch,
        typography::Shaper& shaper,
        const Configuration& config
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), cache(arena, 2048), config(config) {}

    Paragraph* Document::append(
        std::string_view text,
        const typography::Font& font,
        float size
    ) noexcept {
        auto* item = arena.compose<Paragraph>(arena, text, font, size);
        auto* link = arena.compose<Element>();
        link->item = item;
        link->next = nullptr;

        if (!head) {
            head = link;
            tail = link;
        } else {
            tail->next = link;
            tail = link;
        }

        ++count;
        return item;
    }

    void Document::layout() noexcept {
        const float width = config.width - config.left - config.right;
        float top = config.top;
        float shift = 0.0f;

        const Element* current = head;
        while (current) {
            Paragraph* item = current->item;
            if (!item) {
                current = current->next;
                continue;
            }

            if (item->dirty()) {
                const float delta = item->layout(
                    shaper,
                    cache,
                    scratch,
                    width,
                    config.leading
                );
                shift += delta;
                item->offset(top);
            } else {
                if (shift != 0.0f) {
                    item->offset(item->offset() + shift);
                }
            }

            top = item->offset() + item->height() + config.leading;
            current = current->next;
        }
    }

    memory::Slice<Paragraph*> Document::paragraphs() const noexcept {
        if (count == 0) return {};

        auto slice = arena.allocate<Paragraph*>(count);
        const Element* current = head;
        std::size_t step = 0;

        while (current && step < count) {
            slice[step++] = current->item;
            current = current->next;
        }

        return slice;
    }

    const Document::Configuration& Document::configuration() const noexcept {
        return config;
    }

}