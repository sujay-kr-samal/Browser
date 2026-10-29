#include "css/parser.hpp"
#include "css/resolver.hpp"
#include "html/parser.hpp"
#include "layout/box.hpp"
#include "layout/tree_builder.hpp"
#include "paint/painter.hpp"
#include "paint/text_renderer.hpp"

#include <SDL3/SDL.h>
#include <cassert>
#include <iostream>

using namespace divine;

void test_paint_background_and_borders() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    assert(SDL_CreateWindowAndRenderer("Test Paint", 400, 300, 0, &window, &renderer));

    const std::string html =
        "<div style=\"width: 120px; height: 80px; background-color: rgb(255, 0, 0); "
        "border: 2px solid rgb(0, 255, 0); margin: 20px;\"></div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);
    auto layout_root = layout::LayoutTreeBuilder::build(*doc, styles);
    assert(layout_root != nullptr);

    layout::Dimensions cb;
    cb.content.width = 400.0f;
    layout_root->layout(cb);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    paint::PaintOptions options{
        .scroll_x = 0.0f,
        .scroll_y = 0.0f,
        .viewport = layout::Rect{0.0f, 0.0f, 400.0f, 300.0f}
    };
    paint::Painter::paint(renderer, *layout_root, options);
    SDL_RenderPresent(renderer);

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    assert(surface != nullptr);

    Uint8 r = 0, g = 0, b = 0, a = 0;

    // Check interior of box at (x = 20 + 2 + 10 = 32, y = 20 + 2 + 10 = 32)
    // Expected: red background (255, 0, 0)
    assert(SDL_ReadSurfacePixel(surface, 32, 32, &r, &g, &b, &a));
    assert(r == 255 && g == 0 && b == 0);

    // Check border at (x = 20, y = 20)
    // Expected: green border (0, 255, 0)
    assert(SDL_ReadSurfacePixel(surface, 20, 20, &r, &g, &b, &a));
    assert(r == 0 && g == 255 && b == 0);

    // Check outside at (x = 5, y = 5)
    // Expected: clear background white (255, 255, 255)
    assert(SDL_ReadSurfacePixel(surface, 5, 5, &r, &g, &b, &a));
    assert(r == 255 && g == 255 && b == 255);

    SDL_DestroySurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[PASS] test_paint_background_and_borders\n";
}

void test_paint_text() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    assert(SDL_CreateWindowAndRenderer("Test Text Paint", 400, 300, 0, &window, &renderer));

    const std::string html =
        "<p style=\"color: rgb(0, 0, 255); font-size: 24px; margin: 10px;\">TEXT</p>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);
    auto layout_root = layout::LayoutTreeBuilder::build(*doc, styles);
    paint::TextRenderer text_renderer(renderer);
    if (text_renderer.available()) {
        assert(text_renderer.measure_text("iiii", 24.0f, 400).width <
               text_renderer.measure_text("WWWW", 24.0f, 400).width);
    }

    layout::Dimensions cb;
    cb.content.width = 400.0f;
    layout_root->layout(cb, &text_renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    paint::PaintOptions options{
        .scroll_x = 0.0f,
        .scroll_y = 0.0f,
        .viewport = layout::Rect{0.0f, 0.0f, 400.0f, 300.0f},
        .text_renderer = &text_renderer
    };
    paint::Painter::paint(renderer, *layout_root, options);
    SDL_RenderPresent(renderer);

    SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr);
    assert(surface != nullptr);

    // Sample across the text region to find rendered blue glyph pixels
    bool found_blue = false;
    for (int y = 10; y < 35; ++y) {
        for (int x = 10; x < 60; ++x) {
            Uint8 r = 0, g = 0, b = 0, a = 0;
            SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
            if (b > 200 && r < 50 && g < 50) {
                found_blue = true;
                break;
            }
        }
        if (found_blue) break;
    }
    if (text_renderer.available()) {
        assert(found_blue && "Rendered text must produce blue glyph pixels");
    }

    paint::TextRenderer missing_font(renderer, "/missing/divine-test-font.ttf");
    assert(!missing_font.available());
    assert(missing_font.measure_text("safe failure", 16.0f, 400).width == 0.0f);

    SDL_DestroySurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[PASS] test_paint_text\n";
}

void test_paint_viewport_scrolling() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    assert(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    assert(SDL_CreateWindowAndRenderer("Test Scroll", 400, 300, 0, &window, &renderer));

    const std::string html =
        "<div id=\"top-box\" style=\"height: 150px; background-color: rgb(255, 0, 0); margin: 0;\"></div>\n"
        "<div id=\"bottom-box\" style=\"height: 150px; background-color: rgb(0, 255, 0); margin: 0;\"></div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);
    auto layout_root = layout::LayoutTreeBuilder::build(*doc, styles);

    layout::Dimensions cb;
    cb.content.width = 400.0f;
    layout_root->layout(cb);

    // 1. Paint at scroll_y = 0.0f: Top box is in view at y=50..200
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    paint::PaintOptions opt_top{
        .scroll_x = 0.0f,
        .scroll_y = 0.0f,
        .viewport = layout::Rect{0.0f, 50.0f, 400.0f, 250.0f}
    };
    paint::Painter::paint(renderer, *layout_root, opt_top);
    SDL_RenderPresent(renderer);

    SDL_Surface* s_top = SDL_RenderReadPixels(renderer, nullptr);
    assert(s_top != nullptr);
    Uint8 r = 0, g = 0, b = 0, a = 0;
    assert(SDL_ReadSurfacePixel(s_top, 50, 100, &r, &g, &b, &a));
    assert(r == 255 && g == 0 && "Pixel at y=100 must be red at scroll_y = 0");
    SDL_DestroySurface(s_top);

    // 2. Paint at scroll_y = 150.0f: Bottom box is scrolled into position at y=50..200
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    paint::PaintOptions opt_bottom{
        .scroll_x = 0.0f,
        .scroll_y = 150.0f,
        .viewport = layout::Rect{0.0f, 50.0f, 400.0f, 250.0f}
    };
    paint::Painter::paint(renderer, *layout_root, opt_bottom);
    SDL_RenderPresent(renderer);

    SDL_Surface* s_bottom = SDL_RenderReadPixels(renderer, nullptr);
    assert(s_bottom != nullptr);
    assert(SDL_ReadSurfacePixel(s_bottom, 50, 100, &r, &g, &b, &a));
    assert(g == 255 && r == 0 && "Pixel at y=100 must be green at scroll_y = 150");
    SDL_DestroySurface(s_bottom);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[PASS] test_paint_viewport_scrolling\n";
}

int main() {
    std::cout << "Running Divine Engine Paint & Viewport Scrolling Tests...\n";
    test_paint_background_and_borders();
    test_paint_text();
    test_paint_viewport_scrolling();
    std::cout << "All Paint & Viewport Scrolling Tests Passed Successfully!\n";
    return 0;
}
