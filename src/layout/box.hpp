#pragma once

#include "css/types.hpp"
#include "dom/node.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace divine::layout {

struct Rect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    bool operator==(const Rect& other) const = default;
};

struct EdgeDimensions {
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float left{0.0f};

    bool operator==(const EdgeDimensions& other) const = default;
};

struct Dimensions {
    Rect content{};
    EdgeDimensions padding{};
    EdgeDimensions border{};
    EdgeDimensions margin{};

    [[nodiscard]] Rect content_box() const noexcept {
        return content;
    }

    [[nodiscard]] Rect padding_box() const noexcept {
        return Rect{
            .x = content.x - padding.left,
            .y = content.y - padding.top,
            .width = content.width + padding.left + padding.right,
            .height = content.height + padding.top + padding.bottom
        };
    }

    [[nodiscard]] Rect border_box() const noexcept {
        return Rect{
            .x = content.x - padding.left - border.left,
            .y = content.y - padding.top - border.top,
            .width = content.width + padding.left + padding.right + border.left + border.right,
            .height = content.height + padding.top + padding.bottom + border.top + border.bottom
        };
    }

    [[nodiscard]] Rect margin_box() const noexcept {
        return Rect{
            .x = content.x - padding.left - border.left - margin.left,
            .y = content.y - padding.top - border.top - margin.top,
            .width = content.width + padding.left + padding.right + border.left + border.right + margin.left + margin.right,
            .height = content.height + padding.top + padding.bottom + border.top + border.bottom + margin.top + margin.bottom
        };
    }

    bool operator==(const Dimensions& other) const = default;
};

struct TextMetrics {
    float width{0.0f};
    float height{0.0f};
    float ascent{0.0f};
};

class TextMeasurer {
public:
    virtual ~TextMeasurer() = default;
    [[nodiscard]] virtual TextMetrics measure_text(
        std::string_view text, float font_size, uint16_t font_weight) const = 0;
};

enum class BoxType {
    Block,
    Inline,
    Text,
    Anonymous
};

class LayoutBox {
public:
    explicit LayoutBox(BoxType box_type, const dom::Node* node = nullptr, const css::ComputedStyle* style = nullptr);

    // Non-copyable, movable
    LayoutBox(const LayoutBox&) = delete;
    LayoutBox& operator=(const LayoutBox&) = delete;
    LayoutBox(LayoutBox&&) noexcept = default;
    LayoutBox& operator=(LayoutBox&&) noexcept = default;

    [[nodiscard]] BoxType box_type() const noexcept { return m_box_type; }
    [[nodiscard]] const dom::Node* node() const noexcept { return m_node; }
    [[nodiscard]] const css::ComputedStyle& style() const noexcept { return m_style; }
    void set_style(const css::ComputedStyle& style) { m_style = style; }

    [[nodiscard]] const Dimensions& dimensions() const noexcept { return m_dimensions; }
    Dimensions& dimensions() noexcept { return m_dimensions; }

    [[nodiscard]] LayoutBox* parent() const noexcept { return m_parent; }
    [[nodiscard]] const std::vector<std::unique_ptr<LayoutBox>>& children() const noexcept { return m_children; }

    LayoutBox* append_child(std::unique_ptr<LayoutBox> child);

    [[nodiscard]] const std::string& text() const noexcept { return m_text; }
    void set_text(std::string text) { m_text = std::move(text); }

    // Absolute coordinates in viewport
    [[nodiscard]] Rect absolute_content_box() const noexcept;
    [[nodiscard]] Rect absolute_border_box() const noexcept;

    // Layout execution
    void layout(const Dimensions& containing_block, const TextMeasurer* text_measurer = nullptr);

private:
    void layout_block(const Dimensions& containing_block, const TextMeasurer* text_measurer);
    void layout_inline(const Dimensions& containing_block, const TextMeasurer* text_measurer);
    void layout_text(const Dimensions& containing_block, const TextMeasurer* text_measurer);

    void calculate_block_width(const Dimensions& containing_block);
    void calculate_block_position(const Dimensions& containing_block);
    float layout_block_children(const TextMeasurer* text_measurer);
    void calculate_block_height(float cursor_y);

    BoxType m_box_type{BoxType::Block};
    const dom::Node* m_node{nullptr};
    css::ComputedStyle m_style{};
    Dimensions m_dimensions{};
    std::string m_text{};

    LayoutBox* m_parent{nullptr};
    std::vector<std::unique_ptr<LayoutBox>> m_children{};
};

std::string dump_layout_tree(const LayoutBox& root, int depth = 0);
[[nodiscard]] float document_height(const LayoutBox& root) noexcept;

} // namespace divine::layout
