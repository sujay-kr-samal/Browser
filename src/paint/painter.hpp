#pragma once

#include "layout/box.hpp"
#include "paint/text_renderer.hpp"
#include <SDL3/SDL.h>

namespace divine::paint {

struct PaintOptions {
    float scroll_x{0.0f};
    float scroll_y{0.0f};
    layout::Rect viewport{0.0f, 50.0f, 1000.0f, 650.0f};
    TextRenderer* text_renderer{nullptr};
};

class Painter {
public:
    static void paint(
        SDL_Renderer* renderer,
        const layout::LayoutBox& root,
        const PaintOptions& options = {});

private:
    static void paint_box(
        SDL_Renderer* renderer,
        const layout::LayoutBox& box,
        const PaintOptions& options);
};

} // namespace divine::paint
