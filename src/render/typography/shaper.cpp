#include "typography/shaper.hpp"

namespace render::typography {

    memory::Slice<layout::Node*> Shaper::shape(
        const memory::Slice<const Font*> fonts,
        const std::string_view text,
        const memory::Slice<Feature> features
    ) const {
        if (text.empty() || fonts.empty()) return {};

        const Font* primary = nullptr;
        for (std::size_t index = 0; index < fonts.count; ++index) {
            if (fonts[index] && fonts[index]->hb()) {
                primary = fonts[index];
                break;
            }
        }
        if (!primary) return {};

        hb_font_t* handle = primary->hb();
        if (!handle) return {};

        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
        hb_buffer_guess_segment_properties(buffer);

        memory::Slice<hb_feature_t> list = arena.allocate<hb_feature_t>(features.count);
        for (std::size_t index = 0; index < features.count; ++index) {
            list[index] = features[index].raw;
        }

        hb_shape(
            handle,
            buffer,
            list.empty() ? nullptr : list.data,
            static_cast<unsigned int>(list.count)
        );

        unsigned int count = 0;
        const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &count);
        const hb_glyph_position_t* position = hb_buffer_get_glyph_positions(buffer, &count);

        if (count == 0) {
            hb_buffer_destroy(buffer);
            return {};
        }

        memory::Slice<layout::Node*> result = arena.allocate<layout::Node*>(count);
        constexpr float ratio = 1.0f / 64.0f;
        const Font::Metric primary_metric = primary->metrics();

        for (std::size_t index = 0; index < count; ++index) {
            const std::uint32_t cluster = info[index].cluster;
            const char symbol = cluster < text.size() ? text[cluster] : '\0';
            const float advance = static_cast<float>(position[index].x_advance) * ratio;

            if (symbol == ' ') {
                auto* node = arena.compose<layout::Node>(layout::Node::Type::Glue);
                node->glue({
                    .width = advance,
                    .stretch = advance * 0.5f,
                    .shrink = advance * 0.333333f,
                    .expand = layout::Node::Order::Normal,
                    .limit = layout::Node::Order::Normal
                });
                result[index] = node;
                continue;
            }

            std::uint32_t code = info[index].codepoint;
            const Font* font = primary;
            Font::Metric metric = primary_metric;

            if (code == 0 && fonts.count > 1) {
                for (std::size_t step = 0; step < fonts.count; ++step) {
                    const Font* fallback = fonts[step];
                    if (!fallback || fallback == primary || !fallback->hb()) continue;

                    std::uint32_t resolved = 0;
                    if (hb_font_get_nominal_glyph(fallback->hb(), symbol, &resolved) && resolved != 0) {
                        code = resolved;
                        font = fallback;
                        metric = fallback->metrics();
                        break;
                    }
                }
            }

            auto* node = arena.compose<layout::Node>(layout::Node::Type::Glyph);
            node->glyph({
                .width = advance,
                .height = metric.ascent,
                .depth = metric.descent,
                .x = static_cast<float>(position[index].x_offset) * ratio,
                .y = static_cast<float>(position[index].y_offset) * ratio,
                .code = code,
                .font = font
            });
            result[index] = node;
        }

        hb_buffer_destroy(buffer);
        return result;
    }

}