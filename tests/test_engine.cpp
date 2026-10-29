#include "engine.hpp"

#include <cassert>
#include <iostream>

void test_initialization() {
    divine::EngineConfig config{
        .title = "Test Window",
        .width = 800,
        .height = 600,
        .target_fps = 60,
        .max_frames = 1,
        .headless = true
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
        .headless = true
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
        .headless = true
    };

    divine::Engine engine;
    assert(engine.init(config));
    engine.run();

    assert(engine.frame_count() == 15 && "Engine must execute exactly the requested number of frames");
    assert(!engine.is_running() && "Engine should stop after reaching max_frames");

    std::cout << "[PASS] test_bounded_frame_loop\n";
}

int main() {
    std::cout << "Running Divine Engine tests...\n";
    test_initialization();
    test_render_and_pixel_verification();
    test_bounded_frame_loop();
    std::cout << "All Divine Engine tests passed successfully!\n";
    return 0;
}
