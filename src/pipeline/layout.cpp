#include "pipeline/layout.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace pipeline::layout {

    Widget* initialize(core::arena::Arena& arena, const core::ast::Node* source, const Properties& properties) {
        if (source == nullptr) {
            return nullptr;
        }

        if (source->type == core::ast::Type::MACRO && source->value == "\\documentclass") {
            return nullptr;
        }

        auto* widget = arena.construct<Widget>();
        Properties style = properties;
        widget->text = "";

        if (source->type == core::ast::Type::DOCUMENT) {
            style.direction = Direction::COLUMN;
            widget->properties = style;
        } else if (source->type == core::ast::Type::PARAGRAPH) {
            widget->text = "";
            widget->height = style.size * 1.5f;
            widget->properties = style;
            return widget;
        } else if (source->type == core::ast::Type::TEXT) {
            widget->text = source->value;
            widget->properties = style;
        } else if (source->type == core::ast::Type::VERBATIM) {
            style.family = "monospace";
            style.direction = Direction::COLUMN;
            widget->properties = style;
        } else if (source->type == core::ast::Type::DISPLAY) {
            style.direction = Direction::COLUMN;
            widget->properties = style;
        } else if (source->type == core::ast::Type::INLINE) {
            style.direction = Direction::ROW;
            widget->properties = style;
        } else if (source->type == core::ast::Type::MACRO) {
            if (source->value == "\\bold" || source->value == "\\textbf") {
                style.variant = "bold";
            } else if (source->value == "\\italic" || source->value == "\\textit") {
                style.variant = "italic";
            } else if (source->value == "\\sans" || source->value == "\\textsf") {
                style.family = "sans";
            } else if (source->value == "\\mono" || source->value == "\\texttt") {
                style.family = "monospace";
            } else if (source->value == "\\size" && !source->nodes.empty()) {
                if (const core::ast::Node* argument = source->nodes.front(); argument != nullptr) {
                    float value = 0.0f;
                    std::string text(argument->value);
                    if (argument->type == core::ast::Type::GROUP && !argument->nodes.empty()) {
                        text = std::string(argument->nodes.front()->value);
                    }
                    for (char letter : text) {
                        if (letter >= '0' && letter <= '9') {
                            value = value * 10.0f + static_cast<float>(letter - '0');
                        }
                    }
                    if (value > 0.0f) {
                        style.size = value;
                    }
                }
            }
            widget->properties = style;
        }

        std::vector<Widget*> children;
        children.reserve(source->nodes.size());

        size_t skip = 0;
        if (source->type == core::ast::Type::MACRO && source->value == "\\size") {
            skip = 1;
        }

        for (size_t index = skip; index < source->nodes.size(); ++index) {
            const core::ast::Node* child = source->nodes[index];
            if (Widget* item = initialize(arena, child, style); item != nullptr) {
                children.push_back(item);
            }
        }
        widget->children = std::move(children);
        return widget;
    }

    void sizing(Widget* widget, const font::Font& font, const raster::Raster& raster, core::arena::Arena& arena) {
        if (widget == nullptr) {
            return;
        }

        for (Widget* child : widget->children) {
            sizing(child, font, raster, arena);
        }

        if (!widget->text.empty() && widget->children.empty()) {
            std::vector<Widget*> words;
            std::string token;
            std::string text(widget->text);

            for (size_t index = 0; index <= text.size(); ++index) {
                if (index == text.size() || text[index] == ' ' || text[index] == '\n' || text[index] == '\t') {
                    if (!token.empty()) {
                        auto* word = arena.construct<Widget>();
                        auto* pointer = arena.construct<std::string>(token);
                        word->text = *pointer;
                        word->properties = widget->properties;
                        words.push_back(word);
                        token = "";
                    }
                } else {
                    token += text[index];
                }
            }

            if (!words.empty()) {
                float span = 468.0f;
                if (widget->properties.has_value() && widget->properties->width > 0.0f) {
                    span = widget->properties->width;
                }

                std::vector<Widget*> lines;
                auto* line = arena.construct<Widget>();
                Properties style;
                style.direction = Direction::ROW;
                line->properties = style;

                float width = 0.0f;
                float height = 0.0f;

                for (Widget* word : words) {
                    const Properties& trait = word->properties.value_or(Properties{});
                    word->width = font.width(trait.family, trait.variant, std::string(word->text), trait.size);
                    word->height = font.height(trait.family, trait.variant, trait.size);

                    float space = font.width(trait.family, trait.variant, " ", trait.size);

                    if (!line->children.empty() && width + space + word->width > span) {
                        line->width = width;
                        line->height = height * 1.25f;
                        lines.push_back(line);

                        line = arena.construct<Widget>();
                        line->properties = style;
                        width = 0.0f;
                        height = 0.0f;
                    }

                    if (!line->children.empty()) {
                        auto* spacer = arena.construct<Widget>();
                        auto* pointer = arena.construct<std::string>(" ");
                        spacer->text = *pointer;
                        spacer->properties = word->properties;
                        spacer->width = space;
                        spacer->height = word->height;
                        line->children.push_back(spacer);
                        width += space;
                    }

                    line->children.push_back(word);
                    width += word->width;
                    height = std::max(height, word->height);
                }

                if (!line->children.empty()) {
                    line->width = width;
                    line->height = height * 1.25f;
                    lines.push_back(line);
                }

                widget->children = std::move(lines);
                widget->text = "";
            }
        }

        float width = 0.0f;
        float height = 0.0f;
        bool vertical = (!widget->properties.has_value() || widget->properties->direction == Direction::COLUMN);

        for (const Widget* child : widget->children) {
            if (vertical) {
                height += child->height;
                width = std::max(width, child->width);
            } else {
                width += child->width;
                height = std::max(height, child->height);
            }
        }

        widget->width = (width > 0.0f) ? width : widget->width;
        widget->height = (height > 0.0f) ? height : widget->height;
    }

    void position(Widget* widget, float left, float top) {
        if (widget == nullptr) {
            return;
        }

        widget->x = left;
        widget->y = top - widget->height;

        float shift = left;
        float level = top;
        bool vertical = (!widget->properties.has_value() || widget->properties->direction == Direction::COLUMN);

        for (Widget* child : widget->children) {
            position(child, shift, level);
            if (vertical) {
                level -= child->height;
            } else {
                shift += child->width;
            }
        }
    }

    Layout::Layout(core::arena::Arena& arena) : arena(arena) {}

    Widget* Layout::compute(const core::ast::Node* node, const font::Font& font, const raster::Raster& raster) const {
        if (node == nullptr) {
            return nullptr;
        }

        Blueprint blueprint;
        blueprint.width = 612.0f;
        blueprint.height = 792.0f;
        blueprint.margin = 54.0f;
        blueprint.columns = 1;
        blueprint.gutter = 18.0f;
        blueprint.scale = 10.0f;

        for (const core::ast::Node* child : node->nodes) {
            if (child != nullptr && child->type == core::ast::Type::MACRO && child->value == "\\documentclass") {
                for (const core::ast::Node* argument : child->nodes) {
                    if (argument != nullptr) {
                        std::string_view options = argument->value;
                        if (options.find("twocolumn") != std::string_view::npos) {
                            blueprint.columns = 2;
                        }
                        if (options.find("10pt") != std::string_view::npos) {
                            blueprint.scale = 10.0f;
                        } else if (options.find("11pt") != std::string_view::npos) {
                            blueprint.scale = 11.0f;
                        } else if (options.find("12pt") != std::string_view::npos) {
                            blueprint.scale = 12.0f;
                        }
                    }
                }
            }
        }

        Properties style;
        style.family = "serif";
        style.variant = "regular";
        style.size = blueprint.scale;
        style.direction = Direction::COLUMN;

        float width = blueprint.width - (2.0f * blueprint.margin);
        float height = blueprint.height - (2.0f * blueprint.margin);
        float span = width;
        if (blueprint.columns == 2) {
            span = (width - blueprint.gutter) / 2.0f;
        }

        Widget* root = initialize(this->arena, node, style);
        sizing(root, font, raster, this->arena);

        std::vector<Widget*> flow = root->children;
        std::vector<Widget*> pages;

        auto* page = this->arena.construct<Widget>();
        Properties shape;
        shape.direction = Direction::ROW;
        page->properties = shape;
        page->width = blueprint.width;
        page->height = blueprint.height;

        auto* column = this->arena.construct<Widget>();
        Properties format;
        format.direction = Direction::COLUMN;
        column->properties = format;
        column->width = span;
        column->height = height;

        float filled = 0.0f;

        for (Widget* child : flow) {
            if (filled + child->height <= height) {
                column->children.push_back(child);
                filled += child->height;
            } else {
                page->children.push_back(column);
                if (page->children.size() >= static_cast<size_t>(blueprint.columns)) {
                    pages.push_back(page);
                    page = this->arena.construct<Widget>();
                    page->properties = shape;
                    page->width = blueprint.width;
                    page->height = blueprint.height;
                }

                column = this->arena.construct<Widget>();
                column->properties = format;
                column->width = span;
                column->height = height;

                column->children.push_back(child);
                filled = child->height;
            }
        }

        if (!column->children.empty() || !page->children.empty()) {
            if (!column->children.empty()) {
                page->children.push_back(column);
            }
            pages.push_back(page);
        }

        root->children = pages;

        for (Widget* sheet : pages) {
            sheet->x = 0.0f;
            sheet->y = blueprint.height;
            float shift = blueprint.margin;
            for (Widget* block : sheet->children) {
                block->x = shift;
                block->y = blueprint.height - blueprint.margin;
                float level = blueprint.height - blueprint.margin;
                for (Widget* child : block->children) {
                    position(child, shift, level);
                    level -= child->height;
                }
                shift += span + blueprint.gutter;
            }
        }

        return root;
    }

}