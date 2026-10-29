#pragma once

#include "layout/box.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <string_view>

namespace divine::paint {

class TextRenderer final : public layout::TextMeasurer {
public:
    explicit TextRenderer(SDL_Renderer* renderer, std::string_view font_path = {});
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] const std::string& font_path() const noexcept;
    [[nodiscard]] layout::TextMetrics measure_text(
        std::string_view text, float font_size, uint16_t font_weight) const override;
    bool draw_text(
        std::string_view text, float x, float y, float font_size,
        uint16_t font_weight, SDL_Color color);

private:
    struct Impl;
    SDL_Renderer* m_renderer{nullptr};
    std::unique_ptr<Impl> m_impl;
};

} // namespace divine::paint