#include "render/painter.hpp"
#include "typography/face.hpp"
#include "typography/font.hpp"

#include <include/core/SkData.h>
#include <include/core/SkFont.h>
#include <include/core/SkRect.h>
#include <include/core/SkStream.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>

namespace render {

    Painter::Painter(SkCanvas* canvas) noexcept : canvas(canvas) {
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
    }

    void Painter::draw(const layout::Node* root, const float x, const float y) const {
        if (!root) return;

        if (root->type() == layout::Node::Type::Box) {
            box(root, x, y);
        } else {
            node(root, x, y);
        }
    }

    void Painter::box(const layout::Node* node, const float x, const float y) const {
        const auto& information = node->box();
        const bool vertical = (information.alignment == layout::Node::Alignment::Vertical);

        float left = x;
        float top = y;

        for (std::size_t index = 0; index < information.list.count; ++index) {
            const auto* child = information.list[index];
            if (!child) continue;

            if (child->type() == layout::Node::Type::Box) {
                float spot = left;
                float mark = top;

                if (vertical) {
                    spot += child->box().shift;
                } else {
                    mark += child->box().shift;
                }

                box(child, spot, mark);

                if (vertical) {
                    top += child->box().height + child->box().depth;
                } else {
                    left += child->box().width;
                }
            } else {
                this->node(child, left, top);

                if (vertical) {
                    switch (child->type()) {
                        case layout::Node::Type::Glue:
                            top += child->glue().width;
                            break;
                        case layout::Node::Type::Kern:
                            top += child->kern().width;
                            break;
                        case layout::Node::Type::Rule:
                            top += child->rule().height + child->rule().depth;
                            break;
                        case layout::Node::Type::Glyph:
                            top += child->glyph().height + child->glyph().depth;
                            break;
                        default:
                            break;
                    }
                } else {
                    switch (child->type()) {
                        case layout::Node::Type::Glyph:
                            left += child->glyph().width;
                            break;
                        case layout::Node::Type::Glue:
                            left += child->glue().width;
                            break;
                        case layout::Node::Type::Kern:
                            left += child->kern().width;
                            break;
                        case layout::Node::Type::Rule:
                            left += child->rule().width;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    void Painter::node(const layout::Node* node, const float x, const float y) const {
        switch (node->type()) {
            case layout::Node::Type::Glyph:
                glyph(node, x, y);
                break;
            case layout::Node::Type::Rule:
                rule(node, x, y);
                break;
            default:
                break;
        }
    }

    void Painter::glyph(const layout::Node* node, const float x, const float y) const {
        const auto& information = node->glyph();

        SkFont font;
        if (information.font && information.font->face()) {
            const auto* face = information.font->face();
            const auto bytes = face->data();
            if (!bytes.empty()) {
                if (auto data = SkData::MakeWithoutCopy(bytes.data(), bytes.size())) {
                    if (auto stream = SkMemoryStream::MakeForData(std::move(data))) {
                        font.setTypeface(SkTypeface::MakeFromStream(std::move(stream)));
                    }
                }
            }
        }

        const float size = information.font ? information.font->size() : (information.height + information.depth);
        if (size <= 0.0f) return;
        font.setSize(size);

        const SkGlyphID id = static_cast<SkGlyphID>(information.code);
        const SkPoint point = SkPoint::Make(x + information.x, y + information.y);

        if (const auto blob = SkTextBlob::MakeFromPosText(&id, sizeof(SkGlyphID), &point, font); blob) {
            canvas->drawTextBlob(blob, 0, 0, paint);
        }
    }

    void Painter::rule(const layout::Node* node, const float x, const float y) const {
        const auto& [width, height, depth] = node->rule();
        const SkRect rect = SkRect::MakeXYWH(x, y - height, width, height + depth);

        canvas->drawRect(rect, paint);
    }

}