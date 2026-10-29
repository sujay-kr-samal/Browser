#pragma once

#include "layout/box.hpp"
#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace divine {

// RAII custom deleters for SDL3 handles
struct WindowDeleter {
    void operator()(SDL_Window* window) const noexcept {
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
    }
};

struct RendererDeleter {
    void operator()(SDL_Renderer* renderer) const noexcept {
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
    }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

struct EngineConfig {
    std::string title = "Divine Engine";
    int width = 1000;
    int height = 700;
    uint32_t target_fps = 60;
    uint32_t max_frames = 0; // 0 = run indefinitely until quit
    bool headless = false;
};

// Represents a primitive colored rectangle with an optional border
struct RenderRect {
    SDL_FRect rect;
    SDL_Color fill_color;
    bool has_border = false;
    SDL_Color border_color{0, 0, 0, 255};
};

class Engine {
public:
    Engine();
    ~Engine();

    // Non-copyable, movable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;

    // Lifecycle
    bool init(const EngineConfig& config = {});
    void run();
    void stop() noexcept;

    // Page loading and viewport scrolling
    void load_page(std::string_view html, std::string_view css = "");
    void scroll_by(float dx, float dy);
    void scroll_to(float x, float y);
    [[nodiscard]] float scroll_x() const noexcept { return m_scroll_x; }
    [[nodiscard]] float scroll_y() const noexcept { return m_scroll_y; }
    [[nodiscard]] const layout::LayoutBox* layout_root() const noexcept { return m_layout_root.get(); }

    // Frame pipeline
    bool poll_events();
    void render_frame();

    // Accessors
    [[nodiscard]] SDL_Window* window() const noexcept { return m_window.get(); }
    [[nodiscard]] SDL_Renderer* renderer() const noexcept { return m_renderer.get(); }
    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }
    [[nodiscard]] uint64_t frame_count() const noexcept { return m_frame_count; }
    [[nodiscard]] bool is_running() const noexcept { return m_running; }

private:
    void draw_rect(const RenderRect& item);
    void render_chrome();
    void render_page();
    void render_scrollbar(float viewport_top, float viewport_height, float total_height);
    void clamp_scroll();
    void relayout();

    EngineConfig m_config;
    WindowPtr m_window;
    RendererPtr m_renderer;
    int m_width{0};
    int m_height{0};
    bool m_running{false};
    bool m_sdl_initialized{false};
    uint64_t m_frame_count{0};

    std::unique_ptr<layout::LayoutBox> m_layout_root;
    std::string m_html_source;
    std::string m_css_source;
    float m_scroll_x{0.0f};
    float m_scroll_y{0.0f};
};

} // namespace divine
