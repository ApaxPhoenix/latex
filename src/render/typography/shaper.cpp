#include "typography/shaper.hpp"

namespace typography {

    memory::Slice<render::layout::Node*> Shaper::shape(
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

        memory::Slice<render::layout::Node*> result = arena.allocate<render::layout::Node*>(count);
        constexpr float ratio = 1.0f / 64.0f;
        const Font::Metric metric = primary->metrics();

        for (std::size_t index = 0; index < count; ++index) {
            const std::uint32_t cluster = info[index].cluster;
            const char symbol = cluster < text.size() ? text[cluster] : '\0';
            const float advance = static_cast<float>(position[index].x_advance) * ratio;

            if (symbol == ' ') {
                auto* node = arena.compose<render::layout::Node>(render::layout::Node::Type::Glue);
                node->glue({
                    .width = advance,
                    .stretch = advance * 0.5f,
                    .shrink = advance * 0.333333f,
                    .expand = render::layout::Node::Order::Normal,
                    .limit = render::layout::Node::Order::Normal
                });
                result[index] = node;
                continue;
            }

            auto* node = arena.compose<render::layout::Node>(render::layout::Node::Type::Glyph);
            node->glyph({
                .width = advance,
                .height = metric.ascent,
                .depth = metric.descent,
                .x = static_cast<float>(position[index].x_offset) * ratio,
                .y = static_cast<float>(position[index].y_offset) * ratio,
                .code = info[index].codepoint,
                .font = primary
            });
            result[index] = node;
        }

        hb_buffer_destroy(buffer);
        return result;
    }

}