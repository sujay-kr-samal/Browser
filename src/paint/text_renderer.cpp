#include "paint/text_renderer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace divine::paint {

struct TextRenderer::Impl {
    FT_Library library{nullptr};
    FT_Face face{nullptr};
    std::string path;

    ~Impl() {
        if (face != nullptr) FT_Done_Face(face);
        if (library != nullptr) FT_Done_FreeType(library);
    }
};

namespace {

uint32_t next_codepoint(std::string_view text, size_t& offset) {
    const auto first = static_cast<unsigned char>(text[offset++]);
    if (first < 0x80) return first;

    unsigned int continuation_count = 0;
    uint32_t codepoint = 0;
    if ((first & 0xE0) == 0xC0) {
        continuation_count = 1;
        codepoint = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        continuation_count = 2;
        codepoint = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        continuation_count = 3;
        codepoint = first & 0x07;
    } else {
        return 0xFFFD;
    }

    for (unsigned int i = 0; i < continuation_count; ++i) {
        if (offset >= text.size()) return 0xFFFD;
        const auto next = static_cast<unsigned char>(text[offset]);
        if ((next & 0xC0) != 0x80) return 0xFFFD;
        ++offset;
        codepoint = (codepoint << 6) | (next & 0x3F);
    }
    return codepoint;
}

bool set_font_size(FT_Face face, float size) {
    const auto pixels = static_cast<FT_UInt>(std::clamp(std::lround(size), 1L, 512L));
    return FT_Set_Pixel_Sizes(face, 0, pixels) == 0;
}

} // namespace

TextRenderer::TextRenderer(SDL_Renderer* renderer, std::string_view requested_path)
    : m_renderer(renderer), m_impl(std::make_unique<Impl>()) {
    if (FT_Init_FreeType(&m_impl->library) != 0) return;

    std::vector<std::string> candidates;
    if (!requested_path.empty()) candidates.emplace_back(requested_path);
    if (const char* configured = std::getenv("DIVINE_FONT_PATH"); configured != nullptr && *configured != '\0') {
        candidates.emplace_back(configured);
    }
    constexpr std::array<const char*, 5> fallback_paths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/Library/Fonts/Arial.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (const char* path : fallback_paths) candidates.emplace_back(path);

    for (const auto& candidate : candidates) {
        if (!std::filesystem::is_regular_file(candidate)) continue;
        if (FT_New_Face(m_impl->library, candidate.c_str(), 0, &m_impl->face) == 0) {
            m_impl->path = candidate;
            break;
        }
    }
}

TextRenderer::~TextRenderer() = default;

bool TextRenderer::available() const noexcept {
    return m_impl != nullptr && m_impl->face != nullptr && m_renderer != nullptr;
}

const std::string& TextRenderer::font_path() const noexcept {
    return m_impl->path;
}

layout::TextMetrics TextRenderer::measure_text(
    std::string_view text, float font_size, uint16_t font_weight) const {
    if (!available() || !set_font_size(m_impl->face, font_size)) return {};

    float width = 0.0f;
    size_t offset = 0;
    while (offset < text.size()) {
        const auto codepoint = next_codepoint(text, offset);
        const FT_UInt glyph_index = FT_Get_Char_Index(m_impl->face, codepoint);
        if (FT_Load_Glyph(m_impl->face, glyph_index, FT_LOAD_DEFAULT) != 0) continue;
        if (font_weight >= 600) FT_GlyphSlot_Embolden(m_impl->face->glyph);
        width += static_cast<float>(m_impl->face->glyph->advance.x) / 64.0f;
    }

    const auto& metrics = m_impl->face->size->metrics;
    const float height = static_cast<float>(metrics.height) / 64.0f;
    return {
        .width = width,
        .height = height > 0.0f ? height : font_size * 1.25f,
        .ascent = static_cast<float>(metrics.ascender) / 64.0f
    };
}

bool TextRenderer::draw_text(
    std::string_view text, float x, float y, float font_size,
    uint16_t font_weight, SDL_Color color) {
    if (!available() || text.empty() || !set_font_size(m_impl->face, font_size)) return false;

    const auto metrics = measure_text(text, font_size, font_weight);
    const int margin = static_cast<int>(std::ceil(font_size));
    const int width = std::max(1, static_cast<int>(std::ceil(metrics.width)) + margin * 2);
    const int height = std::max(1, static_cast<int>(std::ceil(metrics.height + font_size * 0.5f)));
    const int baseline = static_cast<int>(std::ceil(metrics.ascent + font_size * 0.25f));
    std::vector<Uint8> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0);

    auto blend_pixel = [&](int px, int py, Uint8 coverage) {
        if (px < 0 || px >= width || py < 0 || py >= height || coverage == 0) return;
        const size_t index = (static_cast<size_t>(py) * static_cast<size_t>(width) +
                              static_cast<size_t>(px)) * 4;
        pixels[index] = color.r;
        pixels[index + 1] = color.g;
        pixels[index + 2] = color.b;
        pixels[index + 3] = static_cast<Uint8>((static_cast<unsigned int>(color.a) * coverage) / 255);
    };

    float pen_x = static_cast<float>(margin);
    size_t offset = 0;
    while (offset < text.size()) {
        const auto codepoint = next_codepoint(text, offset);
        const FT_UInt glyph_index = FT_Get_Char_Index(m_impl->face, codepoint);
        if (FT_Load_Glyph(m_impl->face, glyph_index, FT_LOAD_DEFAULT) != 0) continue;
        if (font_weight >= 600) FT_GlyphSlot_Embolden(m_impl->face->glyph);
        if (FT_Render_Glyph(m_impl->face->glyph, FT_RENDER_MODE_NORMAL) != 0) continue;

        const auto& bitmap = m_impl->face->glyph->bitmap;
        for (unsigned int row = 0; row < bitmap.rows; ++row) {
            const int source_row = bitmap.pitch >= 0
                ? static_cast<int>(row)
                : static_cast<int>(bitmap.rows - 1 - row);
            const auto* source = bitmap.buffer + source_row * std::abs(bitmap.pitch);
            for (unsigned int column = 0; column < bitmap.width; ++column) {
                Uint8 coverage = 0;
                if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                    coverage = source[column];
                } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                    coverage = (source[column / 8] & (0x80 >> (column % 8))) ? 255 : 0;
                }
                blend_pixel(
                    static_cast<int>(pen_x) + m_impl->face->glyph->bitmap_left + static_cast<int>(column),
                    baseline - m_impl->face->glyph->bitmap_top + static_cast<int>(row),
                    coverage);
            }
        }
        pen_x += static_cast<float>(m_impl->face->glyph->advance.x) / 64.0f;
    }

    SDL_Surface* raw_surface = SDL_CreateSurfaceFrom(
        width, height, SDL_PIXELFORMAT_RGBA32, pixels.data(), width * 4);
    if (raw_surface == nullptr) return false;
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(raw_surface, SDL_DestroySurface);
    SDL_Texture* raw_texture = SDL_CreateTextureFromSurface(m_renderer, surface.get());
    if (raw_texture == nullptr) return false;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(raw_texture, SDL_DestroyTexture);
    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);

    const SDL_FRect destination{
        x - static_cast<float>(margin), y,
        static_cast<float>(width), static_cast<float>(height)
    };
    return SDL_RenderTexture(m_renderer, texture.get(), nullptr, &destination);
}

} // namespace divine::paint