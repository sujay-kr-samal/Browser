#include "engine.hpp"
#include "css/parser.hpp"
#include "css/resolver.hpp"
#include "html/parser.hpp"
#include "layout/tree_builder.hpp"
#include "paint/painter.hpp"

#include <algorithm>
#include <iostream>

namespace divine {

namespace {

const char* DEFAULT_HTML =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>Divine Engine</title></head>\n"
    "<body>\n"
    "  <div class=\"header\">\n"
    "    <h1>Divine Engine</h1>\n"
    "  </div>\n"
    "  <p class=\"subtitle-line1\">A lightweight, independent browser engine written from scratch in C++20.</p>\n"
    "  <p class=\"subtitle-line2\">Full DOM, CSS Cascader, Box Model Layout, Text Rendering, and Viewport Scrolling.</p>\n"
    "  <div class=\"col-left\">\n"
    "    <div class=\"col-accent-left\"></div>\n"
    "    <p class=\"card-text\">Fast Block and Inline Layout Engine</p>\n"
    "  </div>\n"
    "  <div class=\"col-right\">\n"
    "    <div class=\"col-accent-right\"></div>\n"
    "    <p class=\"card-text\">SDL3 Hardware-Accelerated 2D Painter</p>\n"
    "  </div>\n"
    "  <div class=\"cta-button\">Explore</div>\n"
    "  <div class=\"scroll-section\">\n"
    "    <h2>Interactive Viewport Scrolling</h2>\n"
    "    <p>Scroll down using Mouse Wheel, Up/Down arrow keys, PageUp/PageDown, or Spacebar.</p>\n"
    "    <div class=\"scroll-card\">Card 1: Layout trees accurately translate DOM nodes into screen space.</div>\n"
    "    <div class=\"scroll-card\">Card 2: Sibling boxes stack vertically, accumulating margins without overlaps.</div>\n"
    "    <div class=\"scroll-card\">Card 3: Text rendering scales dynamically using font sizes.</div>\n"
    "    <div class=\"scroll-card\">Card 4: Viewport frustum culling prevents unnecessary draw calls.</div>\n"
    "  </div>\n"
    "</body>\n"
    "</html>";

const char* DEFAULT_CSS =
    "body {\n"
    "    margin: 20px 32px 32px 32px;\n"
    "    background-color: white;\n"
    "    border: 1px solid #dadce0;\n"
    "    padding: 24px;\n"
    "    font-size: 16px;\n"
    "}\n"
    ".header {\n"
    "    background-color: #1a73e8;\n"
    "    color: white;\n"
    "    width: 360px;\n"
    "    height: 32px;\n"
    "    margin-bottom: 12px;\n"
    "}\n"
    "h1 {\n"
    "    font-size: 18px;\n"
    "    color: white;\n"
    "    margin: 0;\n"
    "    padding: 6px 12px;\n"
    "}\n"
    ".subtitle-line1 {\n"
    "    background-color: #9aa0a6;\n"
    "    color: white;\n"
    "    width: 520px;\n"
    "    height: 14px;\n"
    "    margin-bottom: 8px;\n"
    "    font-size: 11px;\n"
    "    padding: 2px 6px;\n"
    "}\n"
    ".subtitle-line2 {\n"
    "    background-color: #dadce0;\n"
    "    color: #3c4043;\n"
    "    width: 440px;\n"
    "    height: 14px;\n"
    "    margin-bottom: 20px;\n"
    "    font-size: 11px;\n"
    "    padding: 2px 6px;\n"
    "}\n"
    ".col-left {\n"
    "    background-color: #e8f0fe;\n"
    "    border: 1px solid #4285f4;\n"
    "    padding: 16px;\n"
    "    margin-bottom: 16px;\n"
    "}\n"
    ".col-accent-left {\n"
    "    background-color: #4285f4;\n"
    "    width: 100px;\n"
    "    height: 20px;\n"
    "    margin-bottom: 8px;\n"
    "}\n"
    ".col-right {\n"
    "    background-color: #e6f4ea;\n"
    "    border: 1px solid #34a853;\n"
    "    padding: 16px;\n"
    "    margin-bottom: 20px;\n"
    "}\n"
    ".col-accent-right {\n"
    "    background-color: #34a853;\n"
    "    width: 100px;\n"
    "    height: 20px;\n"
    "    margin-bottom: 8px;\n"
    "}\n"
    ".cta-button {\n"
    "    background-color: #ea4335;\n"
    "    color: white;\n"
    "    width: 130px;\n"
    "    height: 36px;\n"
    "    font-size: 14px;\n"
    "    padding: 8px 16px;\n"
    "    margin-bottom: 30px;\n"
    "}\n"
    ".scroll-section {\n"
    "    background-color: #f8f9fa;\n"
    "    border: 1px solid #e0e0e0;\n"
    "    padding: 20px;\n"
    "    margin-top: 20px;\n"
    "}\n"
    "h2 {\n"
    "    font-size: 20px;\n"
    "    color: #202124;\n"
    "    margin-bottom: 8px;\n"
    "}\n"
    ".scroll-card {\n"
    "    background-color: white;\n"
    "    border: 1px solid #dadce0;\n"
    "    padding: 14px;\n"
    "    margin-top: 12px;\n"
    "    font-size: 14px;\n"
    "    color: #3c4043;\n"
    "}\n";

} // namespace

Engine::Engine() = default;

Engine::~Engine() {
    m_renderer.reset();
    m_window.reset();
    if (m_sdl_initialized) {
        SDL_Quit();
        m_sdl_initialized = false;
    }
}

Engine::Engine(Engine&& other) noexcept
    : m_config(std::move(other.m_config)),
      m_window(std::move(other.m_window)),
      m_renderer(std::move(other.m_renderer)),
      m_width(other.m_width),
      m_height(other.m_height),
      m_running(other.m_running),
      m_sdl_initialized(other.m_sdl_initialized),
      m_frame_count(other.m_frame_count),
      m_layout_root(std::move(other.m_layout_root)),
      m_html_source(std::move(other.m_html_source)),
      m_css_source(std::move(other.m_css_source)),
      m_scroll_x(other.m_scroll_x),
      m_scroll_y(other.m_scroll_y) {
    other.m_sdl_initialized = false;
    other.m_running = false;
}

Engine& Engine::operator=(Engine&& other) noexcept {
    if (this != &other) {
        m_renderer.reset();
        m_window.reset();
        if (m_sdl_initialized) {
            SDL_Quit();
        }

        m_config = std::move(other.m_config);
        m_window = std::move(other.m_window);
        m_renderer = std::move(other.m_renderer);
        m_width = other.m_width;
        m_height = other.m_height;
        m_running = other.m_running;
        m_sdl_initialized = other.m_sdl_initialized;
        m_frame_count = other.m_frame_count;
        m_layout_root = std::move(other.m_layout_root);
        m_html_source = std::move(other.m_html_source);
        m_css_source = std::move(other.m_css_source);
        m_scroll_x = other.m_scroll_x;
        m_scroll_y = other.m_scroll_y;

        other.m_sdl_initialized = false;
        other.m_running = false;
    }
    return *this;
}

void Engine::load_page(std::string_view html, std::string_view css) {
    m_html_source = html;
    m_css_source = css;

    auto doc = html::Parser::parse_string(m_html_source);
    auto sheet = css::Parser::parse(m_css_source);
    css::StyleResolver resolver(std::move(sheet));
    auto styles = resolver.resolve_document_styles(*doc);

    m_layout_root = layout::LayoutTreeBuilder::build(*doc, styles);
    m_scroll_x = 0.0f;
    m_scroll_y = 0.0f;

    relayout();
}

void Engine::relayout() {
    if (!m_layout_root) return;

    layout::Dimensions cb;
    cb.content.width = static_cast<float>(m_width);
    m_layout_root->layout(cb);
    clamp_scroll();
}

void Engine::clamp_scroll() {
    float max_scroll_y = 0.0f;
    if (m_layout_root) {
        const float content_h = m_layout_root->dimensions().margin_box().height + 70.0f;
        const float vp_h = std::max(100.0f, static_cast<float>(m_height) - 50.0f);
        if (content_h > vp_h) {
            max_scroll_y = content_h - vp_h;
        }
    }
    m_scroll_y = std::clamp(m_scroll_y, 0.0f, max_scroll_y);
    m_scroll_x = 0.0f;
}

void Engine::scroll_by(float dx, float dy) {
    m_scroll_x += dx;
    m_scroll_y += dy;
    clamp_scroll();
}

void Engine::scroll_to(float x, float y) {
    m_scroll_x = x;
    m_scroll_y = y;
    clamp_scroll();
}

bool Engine::init(const EngineConfig& config) {
    m_config = config;

    if (m_config.headless) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[Engine Error] SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }
    m_sdl_initialized = true;

    SDL_Window* raw_window = nullptr;
    SDL_Renderer* raw_renderer = nullptr;

    const SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!SDL_CreateWindowAndRenderer(
            m_config.title.c_str(),
            m_config.width,
            m_config.height,
            window_flags,
            &raw_window,
            &raw_renderer)) {
        std::cerr << "[Engine Error] SDL_CreateWindowAndRenderer failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    m_window.reset(raw_window);
    m_renderer.reset(raw_renderer);

    if (!SDL_SetRenderVSync(m_renderer.get(), 1)) {
        std::cerr << "[Engine Warning] Could not enable VSync: " << SDL_GetError() << '\n';
    }

    SDL_GetRenderOutputSize(m_renderer.get(), &m_width, &m_height);

    // Load default page if none loaded yet
    if (!m_layout_root) {
        load_page(DEFAULT_HTML, DEFAULT_CSS);
    }

    std::cout << "[Engine] Initialized successfully ("
              << m_width << 'x' << m_height
              << ", driver: " << SDL_GetCurrentVideoDriver() << ")\n";

    return true;
}

bool Engine::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                m_running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                SDL_GetRenderOutputSize(m_renderer.get(), &m_width, &m_height);
                relayout();
                break;

            case SDL_EVENT_MOUSE_WHEEL: {
                float dy = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    dy = -dy;
                }
                scroll_by(0.0f, -dy * 40.0f);
                break;
            }

            case SDL_EVENT_KEY_DOWN: {
                const auto key = event.key.key;
                if (key == SDLK_ESCAPE || key == SDLK_Q) {
                    m_running = false;
                } else if (key == SDLK_DOWN || key == SDLK_J) {
                    scroll_by(0.0f, 40.0f);
                } else if (key == SDLK_UP || key == SDLK_K) {
                    scroll_by(0.0f, -40.0f);
                } else if (key == SDLK_PAGEDOWN || key == SDLK_SPACE) {
                    const float vp_h = std::max(100.0f, static_cast<float>(m_height) - 50.0f);
                    scroll_by(0.0f, vp_h * 0.8f);
                } else if (key == SDLK_PAGEUP) {
                    const float vp_h = std::max(100.0f, static_cast<float>(m_height) - 50.0f);
                    scroll_by(0.0f, -vp_h * 0.8f);
                } else if (key == SDLK_HOME) {
                    scroll_to(0.0f, 0.0f);
                } else if (key == SDLK_END) {
                    scroll_to(0.0f, 1000000.0f);
                }
                break;
            }

            default:
                break;
        }
    }
    return m_running;
}

void Engine::draw_rect(const RenderRect& item) {
    SDL_SetRenderDrawColor(
        m_renderer.get(),
        item.fill_color.r,
        item.fill_color.g,
        item.fill_color.b,
        item.fill_color.a
    );
    SDL_RenderFillRect(m_renderer.get(), &item.rect);

    if (item.has_border) {
        SDL_SetRenderDrawColor(
            m_renderer.get(),
            item.border_color.r,
            item.border_color.g,
            item.border_color.b,
            item.border_color.a
        );
        SDL_RenderRect(m_renderer.get(), &item.rect);
    }
}

void Engine::render_chrome() {
    const float fwidth = static_cast<float>(m_width);

    // Top navigation toolbar
    draw_rect({
        .rect = {0.0f, 0.0f, fwidth, 50.0f},
        .fill_color = {241, 243, 244, 255},
        .has_border = true,
        .border_color = {218, 220, 224, 255}
    });

    // Mock navigation buttons (Back, Forward)
    draw_rect({
        .rect = {12.0f, 11.0f, 28.0f, 28.0f},
        .fill_color = {225, 227, 230, 255},
        .has_border = true,
        .border_color = {200, 203, 208, 255}
    });

    draw_rect({
        .rect = {46.0f, 11.0f, 28.0f, 28.0f},
        .fill_color = {225, 227, 230, 255},
        .has_border = true,
        .border_color = {200, 203, 208, 255}
    });

    // Omnibox / address bar
    const float omnibox_width = std::max(120.0f, fwidth - 100.0f);
    draw_rect({
        .rect = {84.0f, 9.0f, omnibox_width, 32.0f},
        .fill_color = {255, 255, 255, 255},
        .has_border = true,
        .border_color = {218, 220, 224, 255}
    });
}

void Engine::render_scrollbar(float viewport_top, float viewport_height, float total_height) {
    if (total_height <= viewport_height) return;

    const float max_scroll = total_height - viewport_height;
    if (max_scroll <= 0.0f) return;

    const float track_w = 8.0f;
    const float track_x = static_cast<float>(m_width) - track_w;
    const float thumb_h = std::max(24.0f, viewport_height * (viewport_height / total_height));
    const float scroll_ratio = std::clamp(m_scroll_y / max_scroll, 0.0f, 1.0f);
    const float thumb_y = viewport_top + scroll_ratio * (viewport_height - thumb_h);

    // Track
    SDL_SetRenderDrawColor(m_renderer.get(), 230, 230, 230, 180);
    const SDL_FRect track_rect{track_x, viewport_top, track_w, viewport_height};
    SDL_RenderFillRect(m_renderer.get(), &track_rect);

    // Thumb
    SDL_SetRenderDrawColor(m_renderer.get(), 170, 170, 170, 220);
    const SDL_FRect thumb_rect{track_x + 1.0f, thumb_y, track_w - 2.0f, thumb_h};
    SDL_RenderFillRect(m_renderer.get(), &thumb_rect);
}

void Engine::render_page() {
    if (!m_layout_root) return;

    const float vp_top = 50.0f;
    const float vp_h = std::max(10.0f, static_cast<float>(m_height) - vp_top);
    const float vp_w = static_cast<float>(m_width);

    paint::PaintOptions options{
        .scroll_x = m_scroll_x,
        .scroll_y = m_scroll_y,
        .viewport = layout::Rect{
            .x = 0.0f,
            .y = vp_top,
            .width = vp_w,
            .height = vp_h
        }
    };

    paint::Painter::paint(m_renderer.get(), *m_layout_root, options);

    const float total_h = m_layout_root->dimensions().margin_box().height + 70.0f;
    render_scrollbar(vp_top, vp_h, total_h);
}

void Engine::render_frame() {
    // Clear canvas to window background color
    SDL_SetRenderDrawColor(m_renderer.get(), 240, 242, 245, 255);
    SDL_RenderClear(m_renderer.get());

    // Paint browser chrome top bar
    render_chrome();

    // Paint the active page layout tree
    render_page();

    // Present buffer to display
    SDL_RenderPresent(m_renderer.get());
}

void Engine::run() {
    m_running = true;
    m_frame_count = 0;

    const uint64_t target_frame_ns = m_config.target_fps > 0
        ? 1'000'000'000ULL / m_config.target_fps
        : 16'666'666ULL;

    while (m_running) {
        const uint64_t frame_start_ns = SDL_GetTicksNS();

        if (!poll_events()) {
            break;
        }

        render_frame();
        ++m_frame_count;

        if (m_config.max_frames > 0 && m_frame_count >= m_config.max_frames) {
            m_running = false;
            break;
        }

        const uint64_t elapsed_ns = SDL_GetTicksNS() - frame_start_ns;
        if (elapsed_ns < target_frame_ns) {
            SDL_DelayNS(target_frame_ns - elapsed_ns);
        }
    }
}

void Engine::stop() noexcept {
    m_running = false;
}

} // namespace divine
