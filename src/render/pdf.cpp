#include "render/pdf.hpp"
#include "render/composer.hpp"

#include <include/core/SkCanvas.h>
#include <include/core/SkStream.h>
#include <include/docs/SkPDFDocument.h>
#include <include/docs/SkPDFJpegHelpers.h>

#include <cstdio>
#include <string>

namespace render {

    bool Pdf::compose(Composer& writer, const float width, const float height, const std::string_view path) {
        if (path.empty() || width <= 0.0f || height <= 0.0f) return false;

        writer.document().layout();

        const auto& pages = writer.engine().compose(writer.document());
        if (pages.empty()) return false;

        SkDynamicMemoryWStream stream;
        SkPDF::Metadata data{};
        data.fTitle = SkString("Document");
        data.jpegDecoder = SkPDF::JPEG::Decode;
        data.jpegEncoder = SkPDF::JPEG::Encode;

        const auto pdf = SkPDF::MakeDocument(&stream, data);
        if (!pdf) return false;

        for (const auto& page : pages) {
            if (SkCanvas* board = pdf->beginPage(width, height)) {
                writer.target(board);
                writer.draw(page.nodes, 0.0f, 0.0f);
                pdf->endPage();
            }
        }

        pdf->close();
        writer.target(nullptr);

        const std::string name(path);
        if (FILE* file = std::fopen(name.c_str(), "wb")) {
            sk_sp<SkData> bytes = stream.detachAsData();
            std::fwrite(bytes->data(), 1, bytes->size(), file);
            std::fclose(file);
            return true;
        }

        return false;
    }

}