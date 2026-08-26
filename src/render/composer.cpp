#include "render/composer.hpp"

#include <include/core/SkData.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkRect.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>
#include <include/ports/SkFontMgr_fontconfig.h>
#include <include/ports/SkFontScanner_FreeType.h>

namespace render {

    Composer::Composer(
        memory::Arena& arena,
        memory::Arena& scratch,
        typography::Shaper& shaper,
        layout::Typesetter& typesetter,
        SkCanvas* canvas
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), typesetter_(typesetter),
          document_(arena, scratch, shaper), _canvas(canvas) {

        ink.setColor(SK_ColorBLACK);
        ink.setAntiAlias(true);
    }

    void Composer::feed(memory::Slice<syntax::Node*> ast, const typography::Font& font, float size) {
        for (const auto& entry : ast) {
            if (entry) {
                document_.append(entry->value, font, size);
            }
        }
    }

    void Composer::feed(const syntax::expression::Node* root, const typography::Font& font) {
        document_.append(root, font);
    }

    void Composer::paint(float horizontal, float vertical) {
        if (!_canvas) return;

        document_.layout();

        for (memory::Slice<layout::Pager::Page> pages = typesetter_.compose(document_); const auto& page : pages) {
            draw(page.nodes, horizontal, vertical);
            vertical += document_.configuration().height;
        }
    }

    void Composer::draw(const layout::Node* root, const float horizontal, const float vertical) const {
        node(root, horizontal, vertical);
    }

    void Composer::draw(const memory::Slice<layout::Node*> nodes, const float horizontal, const float vertical) const {
        stack(nodes, horizontal, vertical);
    }

    void Composer::stack(memory::Slice<layout::Node*> nodes, const float horizontal, const float vertical) const {
        for (const auto* element : nodes) {
            node(element, horizontal, vertical);
        }
    }

    void Composer::box(const layout::Node* item, float x, float y) const {
        if (item->box().alignment == layout::Node::Alignment::Horizontal) {
            y += item->box().shift;
            for (std::size_t index = 0; index < item->box().list.size(); ++index) {
                const auto* child = item->box().list[index];
                node(child, x, y);

                switch (child->type()) {
                    case layout::Node::Type::Box:
                        x += child->box().width;
                        break;
                    case layout::Node::Type::Glyph:
                        x += child->glyph().width;
                        break;
                    case layout::Node::Type::Rule:
                        x += child->rule().width;
                        break;
                    case layout::Node::Type::Kern:
                        x += child->kern().width;
                        break;
                    case layout::Node::Type::Glue: {
                        x += child->glue().width;
                        if (item->box().sign == layout::Node::Sign::Stretching && child->glue().expand == item->box().list[index]->glue().expand) {
                            x += child->glue().stretch * item->box().ratio;
                        } else if (item->box().sign == layout::Node::Sign::Shrinking && child->glue().limit == item->box().list[index]->glue().limit) {
                            x -= child->glue().shrink * item->box().ratio;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        } else {
            x += item->box().shift;
            for (std::size_t index = 0; index < item->box().list.size(); ++index) {
                switch (const auto* child = item->box().list[index]; child->type()) {
                    case layout::Node::Type::Box:
                        y += child->box().height;
                        node(child, x, y);
                        y += child->box().depth;
                        break;
                    case layout::Node::Type::Glyph:
                        y += child->glyph().height;
                        node(child, x, y);
                        y += child->glyph().depth;
                        break;
                    case layout::Node::Type::Rule:
                        y += child->rule().height;
                        node(child, x, y);
                        y += child->rule().depth;
                        break;
                    case layout::Node::Type::Kern:
                        y += child->kern().width;
                        node(child, x, y);
                        break;
                    case layout::Node::Type::Glue: {
                        y += child->glue().width;
                        if (item->box().sign == layout::Node::Sign::Stretching && child->glue().expand == item->box().list[index]->glue().expand) {
                            y += child->glue().stretch * item->box().ratio;
                        } else if (item->box().sign == layout::Node::Sign::Shrinking && child->glue().limit == item->box().list[index]->glue().limit) {
                            y -= child->glue().shrink * item->box().ratio;
                        }
                        node(child, x, y);
                        break;
                    }
                    default:
                        node(child, x, y);
                        break;
                }
            }
        }
    }

    void Composer::node(const layout::Node* item, const float x, const float y) const {
        if (!item) return;

        switch (item->type()) {
            case layout::Node::Type::Box:
                box(item, x, y);
                break;
            case layout::Node::Type::Glyph:
                glyph(item, x, y);
                break;
            case layout::Node::Type::Rule:
                rule(item, x, y);
                break;
            default:
                break;
        }
    }

    void Composer::glyph(const layout::Node* item, float x, float y) const {
        if (!_canvas) return;

        x += item->glyph().x;
        y += item->glyph().y;

        SkFont style;
        if (const auto iterator = fonts.find(item->glyph().font); iterator != fonts.end()) {
            style = iterator->second;
        } else if (item->glyph().font && item->glyph().font->face()) {
            const sk_sp<SkFontMgr> manager = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
            style = SkFont(
                manager->makeFromData(
                    SkData::MakeWithoutCopy(
                        item->glyph().font->face()->data().data(),
                        item->glyph().font->face()->data().size()
                    )
                ),
                item->glyph().font->size()
            );
            style.setEdging(SkFont::Edging::kAntiAlias);
            style.setSubpixel(true);
            fonts[item->glyph().font] = style;
        }

        if (style.getTypeface()) {
            SkTextBlobBuilder builder;
            const auto& stream = builder.allocRunPos(style, 1);
            stream.glyphs[0] = static_cast<SkGlyphID>(item->glyph().code);
            stream.pos[0] = x;
            stream.pos[1] = y;

            _canvas->drawTextBlob(builder.make(), 0.0f, 0.0f, ink);
        }
    }

    void Composer::rule(const layout::Node* item, const float horizontal, const float vertical) const {
        if (!_canvas) return;

        _canvas->drawRect(
            SkRect::MakeLTRB(
                horizontal,
                vertical - item->rule().height,
                horizontal + item->rule().width,
                vertical + item->rule().depth
            ),
            ink
        );
    }

}