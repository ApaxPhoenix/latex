#include "typography/shaper.hpp"

namespace render::typography {

    constexpr Shaper::Utf Shaper::resolve(const std::string_view text, const std::size_t index) noexcept {
        if (index >= text.size()) return {};
        const auto* data = reinterpret_cast<const std::uint8_t*>(text.data() + index);
        const std::size_t rest = text.size() - index;

        if ((data[0] & 0x80) == 0) {
            return {data[0], 1};
        }
        if ((data[0] & 0xE0) == 0xC0 && rest >= 2) {
            return {static_cast<std::uint32_t>((data[0] & 0x1F) << 6 | data[1] & 0x3F), 2};
        }
        if ((data[0] & 0xF0) == 0xE0 && rest >= 3) {
            return {static_cast<std::uint32_t>((data[0] & 0x0F) << 12 | (data[1] & 0x3F) << 6 | data[2] & 0x3F), 3};
        }
        if ((data[0] & 0xF8) == 0xF0 && rest >= 4) {
            return {static_cast<std::uint32_t>((data[0] & 0x07) << 18 | (data[1] & 0x3F) << 12 | (data[2] & 0x3F) << 6 | data[3] & 0x3F), 4};
        }
        return {0, 1};
    }

    memory::Slice<layout::Node*> Shaper::shape(
        const memory::Slice<const Font*> fonts,
        const std::string_view text,
        const memory::Slice<Feature> features
    ) const {
        if (text.empty() || fonts.empty()) return {};

        const Font* main = nullptr;
        for (std::size_t step = 0; step < fonts.count; ++step) {
            if (fonts[step] && fonts[step]->hb()) {
                main = fonts[step];
                break;
            }
        }
        if (!main) return {};

        hb_font_t* handle = main->hb();
        if (!handle) return {};

        hb_buffer_t* buffer = hb_buffer_create();
        hb_buffer_add_utf8(buffer, text.data(), static_cast<int>(text.size()), 0, static_cast<int>(text.size()));
        hb_buffer_guess_segment_properties(buffer);

        memory::Slice<hb_feature_t> list = arena.allocate<hb_feature_t>(features.count);
        for (std::size_t step = 0; step < features.count; ++step) {
            list[step] = features[step].raw;
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

        memory::Slice<layout::Node*> nodes = arena.allocate<layout::Node*>(count);
        constexpr float ratio = 1.0f / 64.0f;
        const Font::Metric base = main->metrics();

        for (std::size_t step = 0; step < count; ++step) {
            const std::uint32_t cluster = info[step].cluster;
            const auto [point, size] = resolve(text, cluster);
            const float advance = static_cast<float>(position[step].x_advance) * ratio;

            if (point == ' ') {
                auto* node = arena.compose<layout::Node>(layout::Node::Type::Glue);
                node->glue({
                    .width = advance,
                    .stretch = advance * 0.5f,
                    .shrink = advance * 0.333333f,
                    .expand = layout::Node::Order::Normal,
                    .limit = layout::Node::Order::Normal
                });
                nodes[step] = node;
                continue;
            }

            std::uint32_t glyph = info[step].codepoint;
            const Font* font = main;
            Font::Metric metric = base;

            if (glyph == 0 && fonts.count > 1) {
                for (std::size_t item = 0; item < fonts.count; ++item) {
                    const Font* fallback = fonts[item];
                    if (!fallback || fallback == main || !fallback->hb()) continue;

                    std::uint32_t resolved = 0;
                    if (hb_font_get_nominal_glyph(fallback->hb(), point, &resolved) && resolved != 0) {
                        glyph = resolved;
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
                .x = static_cast<float>(position[step].x_offset) * ratio,
                .y = static_cast<float>(position[step].y_offset) * ratio,
                .code = glyph,
                .font = font
            });
            nodes[step] = node;
        }

        hb_buffer_destroy(buffer);
        return nodes;
    }

}