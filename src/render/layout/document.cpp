#include "layout/document.hpp"

namespace render::layout {

    Document::Document(
        memory::Arena& arena,
        memory::Arena& scratch,
        typography::Shaper& shaper
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), cache(arena) {}

    Document::Document(
        memory::Arena& arena,
        memory::Arena& scratch,
        typography::Shaper& shaper,
        const Configuration& config
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), cache(arena), config(config) {}

    Paragraph* Document::append(
        const std::string_view text,
        const typography::Font& font,
        const float size
    ) noexcept {
        auto* paragraph = arena.compose<Paragraph>(arena, text, font, size);
        auto* element = arena.compose<Element>();
        element->type = Element::Type::Paragraph;
        element->paragraph = paragraph;

        if (!head) {
            head = element;
        } else {
            tail->next = element;
        }
        tail = element;
        ++count;

        return paragraph;
    }

    void Document::append(
        const syntax::expression::Node* expression,
        const typography::Font& font
    ) noexcept {
        if (!expression) return;

        auto* element = arena.compose<Element>();
        element->type = Element::Type::Expression;
        element->expression = expression;
        element->font = &font;

        if (!head) {
            head = element;
        } else {
            tail->next = element;
        }
        tail = element;
        ++count;
    }

    void Document::layout() noexcept {
        const float target = config.width - config.left - config.right;
        Element* current = head;
        while (current) {
            if (current->type == Element::Type::Paragraph && current->paragraph) {
                current->paragraph->layout(shaper, cache, scratch, target, config.leading);
            }
            current = current->next;
        }
    }

    memory::Slice<Document::Element*> Document::elements() const noexcept {
        if (count == 0) return {};

        auto slice = arena.allocate<Element*>(count);
        Element* current = head;
        std::size_t mark = 0;

        while (current) {
            slice[mark++] = current;
            current = current->next;
        }

        return slice;
    }

    memory::Slice<Paragraph*> Document::paragraphs() const noexcept {
        std::size_t total = 0;
        const Element* current = head;
        while (current) {
            if (current->type == Element::Type::Paragraph) {
                ++total;
            }
            current = current->next;
        }

        if (total == 0) return {};

        auto slice = arena.allocate<Paragraph*>(total);
        current = head;
        std::size_t mark = 0;

        while (current) {
            if (current->type == Element::Type::Paragraph) {
                slice[mark++] = current->paragraph;
            }
            current = current->next;
        }

        return slice;
    }

    const Document::Configuration& Document::configuration() const noexcept {
        return config;
    }

}