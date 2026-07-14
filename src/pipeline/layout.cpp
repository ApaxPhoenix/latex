#include "pipeline/layout.hpp"

#include <algorithm>
#include <string>
#include <cairo.h>

namespace pipeline {

    Layout::Layout(core::arena::Arena& arena) : arena(arena) {}

    Widget* Layout::compute(const core::ast::Node* node, const Font& font, const Raster& raster) const {
        if (!node) return nullptr;

        auto initialize = [&](auto& self, const core::ast::Node* source, const Properties& properties) -> Widget* {
            if (!source) return nullptr;
            auto* widget = this->arena.construct<Widget>();
            Properties current = properties;

            if (source->type == core::ast::Type::PARAGRAPH) {
                current.direction = Direction::COLUMN;
                widget->properties = current;
            } else if (source->type == core::ast::Type::TEXT) {
                widget->text = source->value;
                widget->properties = current;
            } else if (source->type == core::ast::Type::IMAGE) {
                widget->asset = source->value;
                widget->properties = current;
            } else if (source->type == core::ast::Type::MACRO) {
                if (source->value == "\\bold") {
                    current.variant = "bold";
                } else if (source->value == "\\italic") {
                    current.variant = "italic";
                } else if (source->value == "\\regular") {
                    current.variant = "regular";
                } else if (source->value == "\\sans") {
                    current.family = "sans";
                } else if (source->value == "\\serif") {
                    current.family = "serif";
                } else if (source->value == "\\mono") {
                    current.family = "monospace";
                } else if (source->value == "\\column") {
                    current.direction = Direction::COLUMN;
                } else if (source->value == "\\row") {
                    current.direction = Direction::ROW;
                } else if (source->value == "\\size" && !source->nodes.empty()) {
                    const auto* argument = source->nodes.front();
                    if (!argument->nodes.empty() && argument->nodes.front()->type == core::ast::Type::TEXT) {
                        std::string buffer(argument->nodes.front()->value);
                        current.size = std::stof(buffer);
                    }
                } else if (source->value == "\\width" && !source->nodes.empty()) {
                    const auto* argument = source->nodes.front();
                    if (!argument->nodes.empty() && argument->nodes.front()->type == core::ast::Type::TEXT) {
                        std::string buffer(argument->nodes.front()->value);
                        current.width = std::stof(buffer);
                    }
                } else if (source->value == "\\height" && !source->nodes.empty()) {
                    const auto* argument = source->nodes.front();
                    if (!argument->nodes.empty() && argument->nodes.front()->type == core::ast::Type::TEXT) {
                        std::string buffer(argument->nodes.front()->value);
                        current.height = std::stof(buffer);
                    }
                } else {
                    widget->text = source->value;
                }
                widget->properties = current;
            } else if (source->type == core::ast::Type::MATH_DISPLAY) {
                widget->text = "\\mathdisplay";
                current.direction = Direction::ROW;
                widget->properties = current;
            }

            for (const auto* child : source->nodes) {
                if (Widget* branch = self(self, child, current)) {
                    widget->children.push_back(branch);
                }
            }
            return widget;
        };

        auto sizing = [&](auto& self, Widget* widget) -> void {
            if (!widget) return;

            for (auto* child : widget->children) {
                self(self, child);
            }

            if (!widget->text.empty() && widget->properties) {
                const auto& style = widget->properties.value();
                widget->width = font.width(style.family, style.variant, std::string(widget->text), style.size);
                widget->height = font.height(style.family, style.variant, style.size);
            } else if (!widget->asset.empty()) {
                if (auto* graphic = static_cast<cairo_surface_t*>(raster.look(widget->asset))) {
                    widget->width = static_cast<float>(cairo_image_surface_get_width(graphic));
                    widget->height = static_cast<float>(cairo_image_surface_get_height(graphic));
                }
            } else if (widget->properties) {
                float wide = 0.0f;
                float tall = 0.0f;
                for (const auto* child : widget->children) {
                    if (widget->properties->direction == Direction::COLUMN) {
                        tall += child->height;
                        wide = std::max(wide, child->width);
                    } else {
                        wide += child->width;
                        tall = std::max(tall, child->height);
                    }
                }
                widget->width = wide;
                widget->height = tall;
            }

            if (widget->properties) {
                const auto& style = widget->properties.value();
                if (style.width > 0.0f) {
                    widget->width = style.width;
                }
                if (style.height > 0.0f) {
                    widget->height = style.height;
                }
            }
        };

        auto position = [&](auto& self, Widget* widget, float left, float top) -> void {
            if (!widget) return;

            if (widget->text == "\\newpage") {
                top = 792.0f;
                left = 0.0f;
            }

            widget->x = left;
            widget->y = top - widget->height;
            float horizontal = left;
            float vertical = top;

            for (auto* child : widget->children) {
                if (child->text == "\\newpage") {
                    vertical = 792.0f;
                    horizontal = 0.0f;
                }

                float offset = horizontal;
                if (child->text == "\\mathdisplay") {
                    offset = (612.0f - child->width) / 2.0f;
                }

                self(self, child, offset, vertical);

                if (widget->properties && widget->properties->direction == Direction::COLUMN) {
                    vertical -= child->height;
                } else {
                    horizontal += child->width;
                }
            }
        };

        Widget* root = initialize(initialize, node, Properties{});
        sizing(sizing, root);
        position(position, root, 0.0f, 792.0f);
        return root;
    }

}