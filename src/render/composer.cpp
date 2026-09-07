#include "render/composer.hpp"

#include <include/core/SkData.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkRect.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>

#include <cstring>

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
        layout::Typesetter& setter,
        SkCanvas* board
    ) noexcept
        : arena(arena), scratch(scratch), shaper(shaper), setter(setter),
          paper(arena, scratch, shaper), canvas(board) {
        ink.setColor(SK_ColorBLACK);
        ink.setAntiAlias(true);
        glyphs.reserve(1024);
        spots.reserve(1024);
    }

    void Composer::feed(memory::Slice<syntax::Node*> nodes, const typography::Font& font, float size) {
        for (const auto& entry : nodes) {
            if (entry) {
                paper.append(entry->value, font, size);
            }
        }
    }

    void Composer::feed(const syntax::expression::Node* root, const typography::Font& font) {
        if (root) {
            paper.append(root, font);
        }
    }

    void Composer::paint(float across, float down) {
        if (!canvas) return;
        paper.layout();
        for (memory::Slice<layout::Pager::Page> pages = setter.compose(paper); const auto& page : pages) {
            draw(page.nodes, across, down);
            down += paper.configuration().height;
        }
    }

    void Composer::flush() const {
        if (glyphs.empty() || !canvas) return;
        SkTextBlobBuilder maker;
        const auto& run = maker.allocRunPos(style, static_cast<int>(glyphs.size()));
        std::memcpy(run.glyphs, glyphs.data(), glyphs.size() * sizeof(SkGlyphID));
        std::memcpy(run.pos, spots.data(), spots.size() * sizeof(SkPoint));
        if (const sk_sp<SkTextBlob> blob = maker.make()) {
            canvas->drawTextBlob(blob, 0.0f, 0.0f, ink);
        }
        glyphs.clear();
        spots.clear();
    }

    void Composer::draw(const layout::Node* root, const float across, const float down) const {
        node(root, across, down);
        flush();
    }

    void Composer::draw(
        const memory::Slice<layout::Node*> nodes,
        const float across,
        const float down
    ) const {
        stack(nodes, across, down);
        flush();
    }

    void Composer::stack(
        memory::Slice<layout::Node*> nodes,
        const float across,
        const float down
    ) const {
        for (const auto* item : nodes) {
            node(item, across, down);
        }
    }

    void Composer::box(const layout::Node* item, float across, float down) const {
        const auto align = item->box().alignment;
        const auto sign = item->box().sign;
        const auto ratio = item->box().ratio;
        const auto stretch = layout::Node::Sign::Stretching;
        const auto shrink = layout::Node::Sign::Shrinking;

        if (align == layout::Node::Alignment::Horizontal) {
            down += item->box().shift;
            for (const auto* kid : item->box().list) {
                if (!kid) continue;
                node(kid, across, down);
                switch (kid->type()) {
                    case layout::Node::Type::Box:
                        across += kid->box().width;
                        break;
                    case layout::Node::Type::Glyph:
                        across += kid->glyph().width;
                        break;
                    case layout::Node::Type::Rule:
                        across += kid->rule().width;
                        break;
                    case layout::Node::Type::Kern:
                        across += kid->kern().width;
                        break;
                    case layout::Node::Type::Glue:
                        across += kid->glue().width;
                        if (sign == stretch && kid->glue().expand == kid->glue().expand) {
                            across += kid->glue().stretch * ratio;
                        } else if (sign == shrink && kid->glue().limit == kid->glue().limit) {
                            across -= kid->glue().shrink * ratio;
                        }
                        break;
                    default:
                        break;
                }
            }
        } else {
            across += item->box().shift;
            for (const auto* kid : item->box().list) {
                if (!kid) continue;
                switch (kid->type()) {
                    case layout::Node::Type::Box:
                        down += kid->box().height;
                        node(kid, across, down);
                        down += kid->box().depth;
                        break;
                    case layout::Node::Type::Glyph:
                        down += kid->glyph().height;
                        node(kid, across, down);
                        down += kid->glyph().depth;
                        break;
                    case layout::Node::Type::Rule:
                        down += kid->rule().height;
                        node(kid, across, down);
                        down += kid->rule().depth;
                        break;
                    case layout::Node::Type::Kern:
                        down += kid->kern().width;
                        node(kid, across, down);
                        break;
                    case layout::Node::Type::Glue:
                        down += kid->glue().width;
                        if (sign == stretch && kid->glue().expand == kid->glue().expand) {
                            down += kid->glue().stretch * ratio;
                        } else if (sign == shrink && kid->glue().limit == kid->glue().limit) {
                            down -= kid->glue().shrink * ratio;
                        }
                        node(kid, across, down);
                        break;
                    default:
                        node(kid, across, down);
                        break;
                }
            }
        }
    }

    void Composer::node(const layout::Node* item, const float across, const float down) const {
        if (!item) return;
        switch (item->type()) {
            case layout::Node::Type::Box:
                box(item, across, down);
                break;
            case layout::Node::Type::Glyph:
                glyph(item, across, down);
                break;
            case layout::Node::Type::Rule:
                rule(item, across, down);
                break;
            default:
                break;
        }
    }

    void Composer::glyph(const layout::Node* item, float across, float down) const {
        across += item->glyph().x;
        down += item->glyph().y;

        const typography::Font* font = item->glyph().font;
        if (!font || !font->face()) return;

        if (font != cache) {
            flush();
            if (const auto match = styles.find(font); match != styles.end()) {
                style = match->second;
            } else {
                const auto& raw = font->face()->data();
                if (raw.empty()) return;
                const sk_sp<SkData> bytes = SkData::MakeWithoutCopy(raw.data(), raw.size());
                if (!bytes) return;
                static const sk_sp<SkFontMgr> manager = []() {
#if defined(_WIN32)
                    return SkFontMgr_New_DirectWrite();
#else
                    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#endif
                }();
                if (!manager) return;
                sk_sp<SkTypeface> face = manager->makeFromData(bytes);
                if (!face) return;
                style = SkFont(face, font->size());
                style.setEdging(SkFont::Edging::kAntiAlias);
                styles.emplace(font, style);
            }
            cache = font;
        }

        glyphs.emplace_back(static_cast<SkGlyphID>(item->glyph().code));
        spots.emplace_back(SkPoint::Make(across, down));
    }

    void Composer::rule(const layout::Node* item, const float across, const float down) const {
        flush();
        canvas->drawRect(
            SkRect::MakeLTRB(
                across,
                down - item->rule().height,
                across + item->rule().width,
                down + item->rule().depth
            ),
            ink
        );
    }

}