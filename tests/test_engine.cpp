#include "engine.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

void test_initialization() {
    divine::EngineConfig config{
        .title = "Test Window",
        .width = 800,
        .height = 600,
        .target_fps = 60,
        .max_frames = 1,
        .headless = true,
        .font_path = {}
    };

    divine::Engine engine;
    assert(engine.init(config) && "Engine initialization must succeed");
    assert(engine.window() != nullptr && "SDL_Window must not be null");
    assert(engine.renderer() != nullptr && "SDL_Renderer must not be null");
    assert(engine.width() == 800 && "Width should match config");
    assert(engine.height() == 600 && "Height should match config");

    std::cout << "[PASS] test_initialization\n";
}

void test_render_and_pixel_verification() {
    divine::EngineConfig config{
        .title = "Render Test",
        .width = 800,
        .height = 600,
        .target_fps = 60,
        .max_frames = 1,
        .headless = true,
        .font_path = {}
    };

    divine::Engine engine;
    assert(engine.init(config));
    engine.render_frame();

    SDL_Surface* surface = SDL_RenderReadPixels(engine.renderer(), nullptr);
    assert(surface != nullptr && "SDL_RenderReadPixels must succeed");

    Uint8 r = 0, g = 0, b = 0, a = 0;

    // Check heading box at (32 + 24 + 10 = 66, 70 + 24 + 10 = 104)
    // Expected color: RGB(26, 115, 232)
    assert(SDL_ReadSurfacePixel(surface, 66, 104, &r, &g, &b, &a));
    assert(r == 26 && g == 115 && b == 232 && "Heading rectangle color must match #1a73e8");

    // Check document canvas background at (40, 75)
    // Expected color: RGB(255, 255, 255)
    assert(SDL_ReadSurfacePixel(surface, 40, 75, &r, &g, &b, &a));
    assert(r == 255 && g == 255 && b == 255 && "Document canvas background must be white");

    // Check desktop background at (10, 65)
    // Expected color: RGB(240, 242, 245)
    assert(SDL_ReadSurfacePixel(surface, 10, 65, &r, &g, &b, &a));
    assert(r == 240 && g == 242 && b == 245 && "Desktop background color must match #f0f2f5");

    SDL_DestroySurface(surface);
    std::cout << "[PASS] test_render_and_pixel_verification\n";
}

void test_bounded_frame_loop() {
    divine::EngineConfig config{
        .title = "Loop Test",
        .width = 640,
        .height = 480,
        .target_fps = 120,
        .max_frames = 15,
        .headless = true,
        .font_path = {}
    };

    divine::Engine engine;
    assert(engine.init(config));
    engine.run();

    assert(engine.frame_count() == 15 && "Engine must execute exactly the requested number of frames");
    assert(!engine.is_running() && "Engine should stop after reaching max_frames");

    std::cout << "[PASS] test_bounded_frame_loop\n";
}

void test_page_scroll_clamping_and_empty_page() {
    divine::Engine engine;
    divine::EngineConfig config{
        .title = "Scroll Test",
        .width = 400,
        .height = 300,
        .target_fps = 60,
        .max_frames = 1,
        .headless = true,
        .font_path = {}
    };
    assert(engine.init(config));
    engine.load_page(
        "<html><head><style>.red { height: 200px; margin: 0; background-color: red; }"
        ".green { height: 200px; margin: 0; background-color: lime; }</style></head>"
        "<body><div class='red'></div><div class='green'></div></body></html>");
    engine.scroll_to(0.0f, -100.0f);
    assert(engine.scroll_y() == 0.0f);
    engine.render_frame();
    SDL_Surface* top_surface = SDL_RenderReadPixels(engine.renderer(), nullptr);
    assert(top_surface != nullptr);
    Uint8 r = 0, g = 0, b = 0, a = 0;
    assert(SDL_ReadSurfacePixel(top_surface, 10, 100, &r, &g, &b, &a));
    assert(r == 255 && g == 0 && b == 0);
    assert(SDL_ReadSurfacePixel(top_surface, 10, 10, &r, &g, &b, &a));
    assert(r == 241 && g == 243 && b == 244);
    SDL_DestroySurface(top_surface);
    engine.scroll_to(0.0f, 100000.0f);
    assert(engine.scroll_y() > 0.0f);
    const float bottom = engine.scroll_y();
    engine.scroll_by(0.0f, 10000.0f);
    assert(engine.scroll_y() == bottom);
    engine.render_frame();
    SDL_Surface* bottom_surface = SDL_RenderReadPixels(engine.renderer(), nullptr);
    assert(bottom_surface != nullptr);
    assert(SDL_ReadSurfacePixel(bottom_surface, 10, 100, &r, &g, &b, &a));
    assert(r == 0 && g == 255 && b == 0);
    assert(SDL_ReadSurfacePixel(bottom_surface, 10, 10, &r, &g, &b, &a));
    assert(r == 241 && g == 243 && b == 244);
    SDL_DestroySurface(bottom_surface);

    SDL_SetWindowSize(engine.window(), 500, 400);
    SDL_Event resize_event{};
    resize_event.type = SDL_EVENT_WINDOW_RESIZED;
    resize_event.window.windowID = SDL_GetWindowID(engine.window());
    resize_event.window.data1 = 500;
    resize_event.window.data2 = 400;
    SDL_PushEvent(&resize_event);
    engine.poll_events();
    assert(engine.width() == 500 && engine.height() == 400);

    engine.load_page("");
    assert(engine.layout_root() != nullptr);
    engine.scroll_to(0.0f, 1000.0f);
    assert(engine.scroll_y() == 0.0f);

    const auto html_path = std::filesystem::temp_directory_path() / "divine-engine-malformed.html";
    {
        std::ofstream html_file(html_path);
        html_file << "<html><body><p>unfinished";
    }
    assert(!engine.load_file((html_path.string() + ".missing")));
    assert(engine.load_file(html_path.string()));
    assert(engine.layout_root() != nullptr);
    std::filesystem::remove(html_path);

    const auto page_path = std::filesystem::temp_directory_path() / "divine-engine-local-page.html";
    const auto css_path = std::filesystem::temp_directory_path() / "divine-engine-local-page.css";
    {
        std::ofstream css_file(css_path);
        css_file << ".external { height: 60px; margin: 0; background-color: lime; }";
        std::ofstream page_file(page_path);
        page_file << "<html><head><link rel='stylesheet' href='divine-engine-local-page.css'>"
                     "<style>.inline { height: 60px; margin: 0; background-color: red; }</style>"
                     "</head><body><div class='external'></div><div class='inline'></div>"
                     "</body></html>";
    }
    assert(engine.load_file(page_path.string()));
    engine.render_frame();
    SDL_Surface* file_surface = SDL_RenderReadPixels(engine.renderer(), nullptr);
    assert(file_surface != nullptr);
    assert(SDL_ReadSurfacePixel(file_surface, 10, 55, &r, &g, &b, &a));
    assert(r == 0 && g == 255 && b == 0);
    assert(SDL_ReadSurfacePixel(file_surface, 10, 115, &r, &g, &b, &a));
    assert(r == 255 && g == 0 && b == 0);
    SDL_DestroySurface(file_surface);
    std::filesystem::remove(page_path);
    std::filesystem::remove(css_path);
    std::cout << "[PASS] test_page_scroll_clamping_and_empty_page\n";
}

int main() {
    std::cout << "Running Divine Engine tests...\n";
    test_initialization();
    test_render_and_pixel_verification();
    test_bounded_frame_loop();
    test_page_scroll_clamping_and_empty_page();
    std::cout << "All Divine Engine tests passed successfully!\n";
    return 0;
}
