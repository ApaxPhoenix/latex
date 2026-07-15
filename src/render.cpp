#include "render.hpp"

#include <cairo.h>
#include <cairo-pdf.h>
#include <ranges>
#include <vector>


void Render::draw(const layout::Widget* widget, const raster::Raster& raster, const std::string& path) {
    if (!widget) return;

    auto* surface = cairo_pdf_surface_create(path.c_str(), 612.0, 792.0);
    auto* context = cairo_create(surface);

    std::vector<const layout::Widget*> stack;
    stack.push_back(widget);

    while (!stack.empty()) {
        const layout::Widget* item = stack.back();
        stack.pop_back();

        if (!item) continue;

        if (!item->text.empty() && item->properties && item->text != "\\mathdisplay") {
            const auto& style = item->properties.value();
            cairo_select_font_face(context, style.family.c_str(), CAIRO_FONT_SLANT_NORMAL,
                                   style.variant == "bold" ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(context, style.size);
            cairo_move_to(context, item->x, 792.0f - item->y);
            cairo_show_text(context, std::string(item->text).c_str());
        } else if (!item->asset.empty()) {
            if (auto* graphic = static_cast<cairo_surface_t*>(raster.look(item->asset))) {
                cairo_set_source_surface(context, graphic, item->x, 792.0f - item->y - item->height);
                cairo_paint(context);
            }
        }

        for (const auto it : std::views::reverse(item->children)) {
            stack.push_back(it);
        }
    }

    cairo_show_page(context);
    cairo_destroy(context);
    cairo_surface_destroy(surface);
}