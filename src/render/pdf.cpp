#include "render/pdf.hpp"
#include "render/painter.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkStream.h>
#include <include/docs/SkPDFDocument.h>
#include <include/docs/SkPDFJpegHelpers.h>

#include <string>

namespace render {

    bool Pdf::compose(memory::Slice<layout::Node*> pages, std::string_view path) {
        if (path.empty() || pages.empty()) return false;

        const std::string name(path);
        SkFILEWStream stream(name.c_str());
        if (!stream.isValid()) return false;

        SkPDF::Metadata metadata{};
        metadata.fTitle = SkString("Document");
        metadata.jpegDecoder = SkPDF::JPEG::Decode;
        metadata.jpegEncoder = SkPDF::JPEG::Encode;

        const auto doc = SkPDF::MakeDocument(&stream, metadata);
        if (!doc) return false;

        for (const auto* page : pages) {
            if (!page) continue;

            float width = 600.0f;
            float height = 800.0f;

            if (page->type() == layout::Node::Type::Box) {
                const auto& box = page->box();
                if (box.width > 0.0f) width = box.width;
                if (box.height > 0.0f) height = box.height;
            }

            SkCanvas* canvas = doc->beginPage(width, height);
            if (!canvas) continue;

            Painter painter(canvas);
            painter.compose(page);

            doc->endPage();
        }

        doc->close();
        stream.flush();
        return true;
    }

}