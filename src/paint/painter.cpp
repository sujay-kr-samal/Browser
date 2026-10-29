#include "paint/painter.hpp"

#include <algorithm>

namespace divine::paint {

void Painter::paint_box(
    SDL_Renderer* renderer,
    const layout::LayoutBox& box,
    const PaintOptions& options) {

    if (box.style().display == css::Display::None) {
        return;
    }

    const auto border_box = box.absolute_border_box();
    const float draw_x = options.viewport.x + border_box.x - options.scroll_x;
    const float draw_y = options.viewport.y + border_box.y - options.scroll_y;
    const float draw_w = border_box.width;
    const float draw_h = border_box.height;

    // Viewport frustum culling check
    const bool is_visible = !(draw_y + draw_h < options.viewport.y ||
                              draw_y > options.viewport.y + options.viewport.height ||
                              draw_x + draw_w < options.viewport.x ||
                              draw_x > options.viewport.x + options.viewport.width);

    // 1. Paint background fill
    if (is_visible && box.style().background_color.a > 0) {
        SDL_SetRenderDrawColor(
            renderer,
            box.style().background_color.r,
            box.style().background_color.g,
            box.style().background_color.b,
            box.style().background_color.a
        );
        const SDL_FRect bg_rect{draw_x, draw_y, draw_w, draw_h};
        SDL_RenderFillRect(renderer, &bg_rect);
    }

    // 2. Paint borders using each declared side width.
    if (is_visible) {
        const auto& b = box.dimensions().border;
        if (b.top > 0.0f || b.right > 0.0f || b.bottom > 0.0f || b.left > 0.0f) {
            SDL_SetRenderDrawColor(
                renderer,
                box.style().border_color.r,
                box.style().border_color.g,
                box.style().border_color.b,
                box.style().border_color.a
            );
            const SDL_FRect top{draw_x, draw_y, draw_w, b.top};
            const SDL_FRect right{draw_x + draw_w - b.right, draw_y, b.right, draw_h};
            const SDL_FRect bottom{draw_x, draw_y + draw_h - b.bottom, draw_w, b.bottom};
            const SDL_FRect left{draw_x, draw_y, b.left, draw_h};
            if (b.top > 0.0f) SDL_RenderFillRect(renderer, &top);
            if (b.right > 0.0f) SDL_RenderFillRect(renderer, &right);
            if (b.bottom > 0.0f) SDL_RenderFillRect(renderer, &bottom);
            if (b.left > 0.0f) SDL_RenderFillRect(renderer, &left);
        }
    }

    // 3. Paint text content
    if (box.box_type() == layout::BoxType::Text && !box.text().empty()) {
        const auto content_box = box.absolute_content_box();
        const float text_x = options.viewport.x + content_box.x - options.scroll_x;
        const float text_y = options.viewport.y + content_box.y - options.scroll_y;

        if (text_y + content_box.height >= options.viewport.y &&
            text_y <= options.viewport.y + options.viewport.height) {

            if (options.text_renderer != nullptr) {
                const SDL_Color color{
                    box.style().color.r, box.style().color.g,
                    box.style().color.b, box.style().color.a
                };
                options.text_renderer->draw_text(
                    box.text(), text_x, text_y,
                    box.style().font_size > 0.0f ? box.style().font_size : 16.0f,
                    box.style().font_weight, color);
            }
        }
    }

    // 4. Recursively paint children
    for (const auto& child : box.children()) {
        paint_box(renderer, *child, options);
    }
}

void Painter::paint(
    SDL_Renderer* renderer,
    const layout::LayoutBox& root,
    const PaintOptions& options) {

    Uint8 old_r = 0, old_g = 0, old_b = 0, old_a = 0;
    SDL_GetRenderDrawColor(renderer, &old_r, &old_g, &old_b, &old_a);
    float old_scale_x = 1.0f, old_scale_y = 1.0f;
    SDL_GetRenderScale(renderer, &old_scale_x, &old_scale_y);
    const bool had_clip = SDL_RenderClipEnabled(renderer);
    SDL_Rect old_clip{};
    if (had_clip) SDL_GetRenderClipRect(renderer, &old_clip);

    const SDL_Rect clip_rect{
        static_cast<int>(options.viewport.x),
        static_cast<int>(options.viewport.y),
        static_cast<int>(options.viewport.width),
        static_cast<int>(options.viewport.height)
    };
    SDL_SetRenderClipRect(renderer, &clip_rect);

    paint_box(renderer, root, options);

    SDL_SetRenderClipRect(renderer, had_clip ? &old_clip : nullptr);
    SDL_SetRenderScale(renderer, old_scale_x, old_scale_y);
    SDL_SetRenderDrawColor(renderer, old_r, old_g, old_b, old_a);
}

} // namespace divine::paint
