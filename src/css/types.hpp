#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace divine::css {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    bool operator==(const Color& other) const = default;

    static std::optional<Color> parse(std::string_view str);

    static constexpr Color transparent() noexcept { return Color{0, 0, 0, 0}; }
    static constexpr Color black() noexcept { return Color{0, 0, 0, 255}; }
    static constexpr Color white() noexcept { return Color{255, 255, 255, 255}; }
    static constexpr Color red() noexcept { return Color{255, 0, 0, 255}; }
    static constexpr Color green() noexcept { return Color{0, 128, 0, 255}; }
    static constexpr Color blue() noexcept { return Color{0, 0, 255, 255}; }
};

enum class LengthUnit {
    Px,
    Auto
};

struct Length {
    LengthUnit unit{LengthUnit::Px};
    float value{0.0f};

    bool operator==(const Length& other) const = default;

    [[nodiscard]] bool is_auto() const noexcept { return unit == LengthUnit::Auto; }
    [[nodiscard]] bool is_px() const noexcept { return unit == LengthUnit::Px; }

    static Length px(float val) noexcept { return Length{LengthUnit::Px, val}; }
    static Length auto_length() noexcept { return Length{LengthUnit::Auto, 0.0f}; }

    static std::optional<Length> parse(std::string_view str);
};

struct BoxEdges {
    Length top{Length::px(0.0f)};
    Length right{Length::px(0.0f)};
    Length bottom{Length::px(0.0f)};
    Length left{Length::px(0.0f)};

    bool operator==(const BoxEdges& other) const = default;
};

enum class Display {
    Inline,
    Block,
    None
};

enum class TextAlign {
    Left,
    Right,
    Center,
    Justify
};

struct ComputedStyle {
    Color color{Color::black()};
    Color background_color{Color::transparent()};
    Length width{Length::auto_length()};
    Length height{Length::auto_length()};
    BoxEdges margin{};
    BoxEdges padding{};
    BoxEdges border_width{};
    Color border_color{Color::black()};
    Display display{Display::Inline};
    float font_size{16.0f};
    uint16_t font_weight{400};
    TextAlign text_align{TextAlign::Left};

    bool operator==(const ComputedStyle& other) const = default;
};

} // namespace divine::css
