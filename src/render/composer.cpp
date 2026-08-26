#include "render/composer.hpp"

#include <include/core/SkData.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkRect.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>

#if defined(_WIN32)
    #include <include/ports/SkTypeface_win.h>
#else
    #include <include/ports/SkFontMgr_fontconfig.h>
    #include <include/ports/SkFontScanner_FreeType.h>
#endif

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
        if (root) {
            document_.append(root, font);
        }
    }

    void Composer::paint(float x, float y) {
        if (!_canvas) {
            return;
        }

        document_.layout();

        for (memory::Slice<layout::Pager::Page> pages = typesetter_.compose(document_); const auto& page : pages) {
            draw(page.nodes, x, y);
            y += document_.configuration().height;
        }
    }

    void Composer::draw(const layout::Node* root, const float x, const float y) const {
        node(root, x, y);
    }

    void Composer::draw(
        const memory::Slice<layout::Node*> nodes,
        const float x,
        const float y
    ) const {
        stack(nodes, x, y);
    }

    void Composer::stack(
        memory::Slice<layout::Node*> nodes,
        const float x,
        const float y
    ) const {
        for (const auto* element : nodes) {
            node(element, x, y);
        }
    }

    void Composer::box(const layout::Node* item, float x, float y) const {
        if (item->box().alignment == layout::Node::Alignment::Horizontal) {
            y += item->box().shift;

            for (std::size_t index = 0; index < item->box().list.size(); ++index) {
                const auto* child = item->box().list[index];

                if (!child) {
                    continue;
                }

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

                    case layout::Node::Type::Glue:
                        x += child->glue().width;

                        if (
                            item->box().sign == layout::Node::Sign::Stretching &&
                            child->glue().expand == item->box().list[index]->glue().expand
                        ) {
                            x += child->glue().stretch * item->box().ratio;
                        } else if (
                            item->box().sign == layout::Node::Sign::Shrinking &&
                            child->glue().limit == item->box().list[index]->glue().limit
                        ) {
                            x -= child->glue().shrink * item->box().ratio;
                        }

                        break;

                    default:
                        break;
                }
            }
        } else {
            x += item->box().shift;

            for (std::size_t index = 0; index < item->box().list.size(); ++index) {
                const auto* child = item->box().list[index];

                if (!child) {
                    continue;
                }

                switch (child->type()) {
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

                    case layout::Node::Type::Glue:
                        y += child->glue().width;

                        if (
                            item->box().sign == layout::Node::Sign::Stretching &&
                            child->glue().expand == item->box().list[index]->glue().expand
                        ) {
                            y += child->glue().stretch * item->box().ratio;
                        } else if (
                            item->box().sign == layout::Node::Sign::Shrinking &&
                            child->glue().limit == item->box().list[index]->glue().limit
                        ) {
                            y -= child->glue().shrink * item->box().ratio;
                        }

                        node(child, x, y);
                        break;

                    default:
                        node(child, x, y);
                        break;
                }
            }
        }
    }

    void Composer::node(const layout::Node* item, const float x, const float y) const {
        if (!item) {
            return;
        }

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
        x += item->glyph().x;
        y += item->glyph().y;

        const typography::Font* font = item->glyph().font;

        if (!font || !font->face()) {
            return;
        }

        SkFont style;

        if (const auto iterator = fonts.find(font); iterator != fonts.end()) {
            style = iterator->second;
        } else {
            const auto& data = font->face()->data();

            if (data.empty()) {
                return;
            }

            const sk_sp<SkData> fontData = SkData::MakeWithCopy(
                data.data(),
                data.size()
            );

            if (!fontData) {
                return;
            }

            sk_sp<SkTypeface> typeface;

#if defined(_WIN32)
            const sk_sp<SkFontMgr> manager = SkFontMgr_New_DirectWrite();

            if (!manager) {
                return;
            }

            typeface = manager->makeFromData(fontData);
#else
            const sk_sp<SkFontMgr> manager = SkFontMgr_New_FontConfig(
                nullptr,
                SkFontScanner_Make_FreeType()
            );

            if (!manager) {
                return;
            }

            typeface = manager->makeFromData(fontData);
#endif

            if (!typeface) {
                return;
            }

            style = SkFont(
                typeface,
                font->size()
            );

            style.setEdging(SkFont::Edging::kAntiAlias);
            style.setSubpixel(true);

            fonts.emplace(font, style);
        }

        if (!style.getTypeface()) {
            return;
        }

        SkTextBlobBuilder builder;

        const auto& run = builder.allocRunPos(style, 1);

        run.glyphs[0] = static_cast<SkGlyphID>(item->glyph().code);
        run.pos[0] = x;
        run.pos[1] = y;

        const sk_sp<SkTextBlob> blob = builder.make();

        if (!blob) {
            return;
        }

        _canvas->drawTextBlob(
            blob,
            0.0f,
            0.0f,
            ink
        );
    }

    void Composer::rule(
        const layout::Node* item,
        const float x,
        const float y
    ) const {
        _canvas->drawRect(
            SkRect::MakeLTRB(
                x,
                y - item->rule().height,
                x + item->rule().width,
                y + item->rule().depth
            ),
            ink
        );
    }

}