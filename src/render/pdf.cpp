#include "render/pdf.hpp"
#include "render/composer.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkStream.h>
#include <include/docs/SkPDFDocument.h>
#include <include/docs/SkPDFJpegHelpers.h>

#include <string>

namespace render {

    bool Pdf::compose(Composer& composer, const float width, const float height, const std::string_view path) {
        if (path.empty() || width <= 0.0f || height <= 0.0f) return false;

        composer.document().layout();

        const auto pages = composer.typesetter().compose(composer.document());
        if (pages.empty()) return false;

        const std::string name(path);
        SkFILEWStream stream(name.c_str());
        if (!stream.isValid()) return false;

        SkPDF::Metadata metadata{};
        metadata.fTitle = SkString("Document");
        metadata.jpegDecoder = SkPDF::JPEG::Decode;
        metadata.jpegEncoder = SkPDF::JPEG::Encode;

        const auto document = SkPDF::MakeDocument(&stream, metadata);
        if (!document) return false;

        for (const auto& page : pages) {
            SkCanvas* canvas = document->beginPage(width, height);
            if (!canvas) continue;

            composer.canvas(canvas);
            composer.draw(page.nodes, 0.0f, 0.0f);

            document->endPage();
        }

        document->close();
        stream.flush();

        composer.canvas(nullptr);
        return true;
    }

}