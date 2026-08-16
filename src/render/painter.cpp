#include "render/painter.hpp"

#include <include/core/SkFontMgr.h>
#include <include/core/SkTypeface.h>

#if defined(_WIN32)
#include <include/ports/SkTypeface_win.h>
#elif defined(__APPLE__)
#include <include/ports/SkFontMgr_mac_ct.h>
#elif defined(__ANDROID__)
#include <include/ports/SkFontMgr_android.h>
#elif defined(__linux__)
#include <include/ports/SkFontMgr_fontconfig.h>
#endif

namespace render {

    Painter::Painter(SkCanvas* target) noexcept : canvas(target) {
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);

        sk_sp<SkFontMgr> manager;

        #if defined(_WIN32)
            manager = SkFontMgr_New_DirectWrite();
        #elif defined(__APPLE__)
            manager = SkFontMgr_New_CoreText();
        #elif defined(__ANDROID__)
            manager = SkFontMgr_New_Android(nullptr);
        #elif defined(__linux__)
            manager = SkFontMgr_New_FontConfig(nullptr);
        #endif

        if (manager) {
            font.setTypeface(manager->legacyMakeTypeface("Arial", SkFontStyle()));
        }

        font.setSize(12.0f);
    }

    void Painter::compose(const layout::Node* root, float x, float y) {
        for (const layout::Node* node = root; node != nullptr; node = node->next()) {
            switch (node->type()) {
                case layout::Node::Type::Glyph:
                    character(node, x, y);
                    x += node->glyph().width;
                    break;

                case layout::Node::Type::Rule:
                    rule(node, x, y);
                    x += node->rule().width;
                    break;

                case layout::Node::Type::Box:
                    box(node, x, y);
                    x += node->box().width;
                    break;

                case layout::Node::Type::Glue:
                    x += node->glue().width;
                    break;

                case layout::Node::Type::Kern:
                    x += node->kern().width;
                    break;

                default:
                    break;
            }
        }
    }

    void Painter::character(const layout::Node* node, const float x, const float y) const {
        const auto& glyph = node->glyph();
        const char text[2] = {static_cast<char>(glyph.code), '\0'};

        canvas->drawString(text, x, y, font, paint);
    }

    void Painter::rule(const layout::Node* node, const float x, const float y) const {
        const auto&[width, height, depth] = node->rule();
        const SkRect rect = SkRect::MakeXYWH(x, y - height, width, height + depth);

        canvas->drawRect(rect, paint);
    }

    void Painter::box(const layout::Node* node, const float x, const float y) {
        for (const auto* child : node->box().list) {
            compose(child, x, y);
        }
    }

}