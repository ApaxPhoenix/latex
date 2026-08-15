#include "typography/shaper.hpp"

#include <vector>

namespace typography {

    Shaper::Shaper(memory::Arena& arena) noexcept
        : arena(arena) {}

    memory::Slice<layout::Node*> Shaper::shape(
        const Font& font,
        const std::string_view text
    ) const {
        return shape(font, text, {});
    }

    memory::Slice<layout::Node*> Shaper::shape(
        const Font& font,
        const std::string_view text,
        const memory::Slice<Feature> features
    ) const {
        if (text.empty()) {
            return arena.allocate<layout::Node*>(0);
        }

        hb_font_t* handle = font.fetch();
        if (!handle) {
            return arena.allocate<layout::Node*>(0);
        }

        hb_font_extents_t extents{};
        hb_font_get_extents_for_direction(handle, HB_DIRECTION_LTR, &extents);

        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
        hb_buffer_guess_segment_properties(buffer);

        std::vector<hb_feature_t> raw_features;
        raw_features.reserve(features.count);
        for (std::size_t i = 0; i < features.count; ++i) {
            raw_features.push_back(features[i].raw);
        }

        hb_shape(
            handle,
            buffer,
            raw_features.empty() ? nullptr : raw_features.data(),
            static_cast<unsigned int>(raw_features.size())
        );

        unsigned int count = 0;
        const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &count);
        const hb_glyph_position_t* position = hb_buffer_get_glyph_positions(buffer, &count);

        auto result = arena.allocate<layout::Node*>(count);

        for (std::size_t index = 0; index < count; ++index) {
            const std::uint32_t cluster = info[index].cluster;
            const char symbol = cluster < text.size() ? text[cluster] : '\0';

            // HarfBuzz 26.6 coordinate conversion factor (64 units per point/pixel)
            constexpr float scale = 1.0f / 64.0f;
            const float advance = static_cast<float>(position[index].x_advance) * scale;

            if (symbol == ' ') {
                auto* glue_node = arena.compose<layout::Node>(layout::Node::Type::Glue);

                layout::Node::Glue glue_payload{};
                glue_payload.width = advance;
                glue_payload.stretch = advance * 0.5f;
                glue_payload.shrink = advance * 0.333333f;
                glue_payload.stretchorder = layout::Node::Order::Normal;
                glue_payload.shrinkorder = layout::Node::Order::Normal;

                glue_node->glue(glue_payload);
                result[index] = glue_node;
                continue;
            }

            auto* glyph_node = arena.compose<layout::Node>(layout::Node::Type::Glyph);

            layout::Node::Glyph glyph_payload{};
            glyph_payload.font = 1; // Primary font ID
            glyph_payload.code = info[index].codepoint;
            glyph_payload.width = advance;
            glyph_payload.height = static_cast<float>(extents.ascender) * scale;
            glyph_payload.depth = static_cast<float>(-extents.descender) * scale;

            glyph_node->glyph(glyph_payload);
            result[index] = glyph_node;
        }

        hb_buffer_destroy(buffer);
        return result;
    }

}