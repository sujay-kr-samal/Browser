#include "layout/box.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace divine::layout {

LayoutBox::LayoutBox(BoxType box_type, const dom::Node* node, const css::ComputedStyle* style)
    : m_box_type(box_type), m_node(node) {
    if (style != nullptr) {
        m_style = *style;
    }
}

LayoutBox* LayoutBox::append_child(std::unique_ptr<LayoutBox> child) {
    if (!child) return nullptr;
    child->m_parent = this;
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

Rect LayoutBox::absolute_content_box() const noexcept {
    Rect abs_rect = m_dimensions.content;
    const LayoutBox* curr = m_parent;
    while (curr != nullptr) {
        abs_rect.x += curr->dimensions().content.x;
        abs_rect.y += curr->dimensions().content.y;
        curr = curr->m_parent;
    }
    return abs_rect;
}

Rect LayoutBox::absolute_border_box() const noexcept {
    Rect abs_content = absolute_content_box();
    return Rect{
        .x = abs_content.x - m_dimensions.padding.left - m_dimensions.border.left,
        .y = abs_content.y - m_dimensions.padding.top - m_dimensions.border.top,
        .width = abs_content.width + m_dimensions.padding.left + m_dimensions.padding.right +
                 m_dimensions.border.left + m_dimensions.border.right,
        .height = abs_content.height + m_dimensions.padding.top + m_dimensions.padding.bottom +
                  m_dimensions.border.top + m_dimensions.border.bottom
    };
}

void LayoutBox::layout(const Dimensions& containing_block, const TextMeasurer* text_measurer) {
    switch (m_box_type) {
        case BoxType::Block:
        case BoxType::Anonymous:
            layout_block(containing_block, text_measurer);
            break;
        case BoxType::Inline:
            layout_inline(containing_block, text_measurer);
            break;
        case BoxType::Text:
            layout_text(containing_block, text_measurer);
            break;
    }
}

void LayoutBox::calculate_block_width(const Dimensions& containing_block) {
    m_dimensions.padding.top = m_style.padding.top.value;
    m_dimensions.padding.right = m_style.padding.right.value;
    m_dimensions.padding.bottom = m_style.padding.bottom.value;
    m_dimensions.padding.left = m_style.padding.left.value;

    m_dimensions.border.top = m_style.border_width.top.value;
    m_dimensions.border.right = m_style.border_width.right.value;
    m_dimensions.border.bottom = m_style.border_width.bottom.value;
    m_dimensions.border.left = m_style.border_width.left.value;

    const float total_h_extras = m_dimensions.padding.left + m_dimensions.padding.right +
                                 m_dimensions.border.left + m_dimensions.border.right;

    if (m_style.width.is_auto()) {
        m_dimensions.margin.left = m_style.margin.left.is_auto() ? 0.0f : m_style.margin.left.value;
        m_dimensions.margin.right = m_style.margin.right.is_auto() ? 0.0f : m_style.margin.right.value;
        const float available = containing_block.content.width - m_dimensions.margin.left -
                                m_dimensions.margin.right - total_h_extras;
        m_dimensions.content.width = std::max(0.0f, available);
    } else {
        m_dimensions.content.width = std::max(0.0f, m_style.width.value);

        const bool auto_left = m_style.margin.left.is_auto();
        const bool auto_right = m_style.margin.right.is_auto();
        const float remaining = containing_block.content.width - (m_dimensions.content.width + total_h_extras);

        if (auto_left && auto_right) {
            const float half = std::max(0.0f, remaining / 2.0f);
            m_dimensions.margin.left = half;
            m_dimensions.margin.right = half;
        } else if (auto_left) {
            m_dimensions.margin.right = m_style.margin.right.value;
            m_dimensions.margin.left = std::max(0.0f, remaining - m_dimensions.margin.right);
        } else if (auto_right) {
            m_dimensions.margin.left = m_style.margin.left.value;
            m_dimensions.margin.right = std::max(0.0f, remaining - m_dimensions.margin.left);
        } else {
            m_dimensions.margin.left = m_style.margin.left.value;
            m_dimensions.margin.right = m_style.margin.right.value;
        }
    }
}

void LayoutBox::calculate_block_position(const Dimensions& containing_block) {
    m_dimensions.margin.top = m_style.margin.top.is_auto() ? 0.0f : m_style.margin.top.value;
    m_dimensions.margin.bottom = m_style.margin.bottom.is_auto() ? 0.0f : m_style.margin.bottom.value;

    m_dimensions.content.x = m_dimensions.margin.left + m_dimensions.border.left + m_dimensions.padding.left;
    m_dimensions.content.y = containing_block.content.height + m_dimensions.margin.top +
                             m_dimensions.border.top + m_dimensions.padding.top;
}

float LayoutBox::layout_block_children(const TextMeasurer* text_measurer) {
    bool has_block = std::any_of(m_children.begin(), m_children.end(), [](const auto& c) {
        return c->box_type() == BoxType::Block || c->box_type() == BoxType::Anonymous;
    });

    if (has_block) {
        float cursor_y = 0.0f;
        for (auto& child : m_children) {
            Dimensions child_cb;
            child_cb.content.width = m_dimensions.content.width;
            child_cb.content.height = cursor_y;
            child->layout(child_cb, text_measurer);
            cursor_y += child->dimensions().margin_box().height;
        }
        return cursor_y;
    }

    // Inline formatting context for children
    float line_x = 0.0f;
    float line_y = 0.0f;
    float current_line_height = 0.0f;
    const float max_width = m_dimensions.content.width;

    for (auto& child : m_children) {
        Dimensions inline_cb;
        inline_cb.content.width = max_width;
        child->layout(inline_cb, text_measurer);

        const float child_w = child->dimensions().margin_box().width;
        const float child_h = child->dimensions().margin_box().height;

        // Line wrap when exceeding available block width
        if (line_x > 0.0f && line_x + child_w > max_width) {
            line_y += current_line_height;
            line_x = 0.0f;
            current_line_height = 0.0f;
        }

        child->dimensions().content.x = line_x + child->dimensions().margin.left +
                                        child->dimensions().border.left + child->dimensions().padding.left;
        child->dimensions().content.y = line_y + child->dimensions().margin.top +
                                        child->dimensions().border.top + child->dimensions().padding.top;

        line_x += child_w;
        current_line_height = std::max(current_line_height, child_h);
    }

    return line_y + current_line_height;
}

void LayoutBox::calculate_block_height(float cursor_y) {
    if (m_style.height.is_auto()) {
        m_dimensions.content.height = cursor_y;
    } else {
        m_dimensions.content.height = std::max(0.0f, m_style.height.value);
    }
}

void LayoutBox::layout_block(const Dimensions& containing_block, const TextMeasurer* text_measurer) {
    calculate_block_width(containing_block);
    calculate_block_position(containing_block);
    float cursor_y = layout_block_children(text_measurer);
    calculate_block_height(cursor_y);
}

void LayoutBox::layout_inline(const Dimensions& containing_block, const TextMeasurer* text_measurer) {
    m_dimensions.padding.top = m_style.padding.top.value;
    m_dimensions.padding.right = m_style.padding.right.value;
    m_dimensions.padding.bottom = m_style.padding.bottom.value;
    m_dimensions.padding.left = m_style.padding.left.value;

    m_dimensions.border.top = m_style.border_width.top.value;
    m_dimensions.border.right = m_style.border_width.right.value;
    m_dimensions.border.bottom = m_style.border_width.bottom.value;
    m_dimensions.border.left = m_style.border_width.left.value;

    m_dimensions.margin.top = m_style.margin.top.is_auto() ? 0.0f : m_style.margin.top.value;
    m_dimensions.margin.right = m_style.margin.right.is_auto() ? 0.0f : m_style.margin.right.value;
    m_dimensions.margin.bottom = m_style.margin.bottom.is_auto() ? 0.0f : m_style.margin.bottom.value;
    m_dimensions.margin.left = m_style.margin.left.is_auto() ? 0.0f : m_style.margin.left.value;

    float total_width = 0.0f;
    float max_height = 0.0f;

    for (auto& child : m_children) {
        child->layout(containing_block, text_measurer);
        child->dimensions().content.x = total_width + child->dimensions().margin.left;
        child->dimensions().content.y = child->dimensions().margin.top;

        total_width += child->dimensions().margin_box().width;
        max_height = std::max(max_height, child->dimensions().margin_box().height);
    }

    m_dimensions.content.width = total_width;
    m_dimensions.content.height = max_height;
}

void LayoutBox::layout_text(const Dimensions& /*containing_block*/, const TextMeasurer* text_measurer) {
    const float font_size = m_style.font_size > 0.0f ? m_style.font_size : 16.0f;
    if (text_measurer != nullptr) {
        const auto metrics = text_measurer->measure_text(m_text, font_size, m_style.font_weight);
        m_dimensions.content.width = metrics.width;
        m_dimensions.content.height = metrics.height;
    } else {
        m_dimensions.content.width = static_cast<float>(m_text.size()) * font_size * 0.55f;
        m_dimensions.content.height = font_size * 1.25f;
    }
    m_dimensions.padding = {};
    m_dimensions.border = {};
    m_dimensions.margin = {};
}

std::string dump_layout_tree(const LayoutBox& root, int depth) {
    std::ostringstream ss;
    const std::string indent(depth * 2, ' ');

    ss << indent;
    switch (root.box_type()) {
        case BoxType::Block: ss << "Block"; break;
        case BoxType::Inline: ss << "Inline"; break;
        case BoxType::Text: ss << "Text"; break;
        case BoxType::Anonymous: ss << "Anonymous"; break;
    }

    if (root.node() != nullptr) {
        if (root.node()->is_element()) {
            const auto* elem = static_cast<const dom::Element*>(root.node());
            ss << " (" << elem->tag_name();
            if (!elem->id().empty()) ss << '#' << elem->id();
            if (!elem->class_name().empty()) ss << '.' << elem->class_name();
            ss << ')';
        } else if (root.node()->is_text()) {
            ss << " (\"" << root.text() << "\")";
        }
    }

    const auto& d = root.dimensions();
    ss << " [content: (" << d.content.x << ", " << d.content.y << ", "
       << d.content.width << "x" << d.content.height << ")]";

    if (d.margin != EdgeDimensions{}) {
        ss << " margin: (" << d.margin.top << ", " << d.margin.right << ", "
           << d.margin.bottom << ", " << d.margin.left << ")";
    }
    if (d.padding != EdgeDimensions{}) {
        ss << " padding: (" << d.padding.top << ", " << d.padding.right << ", "
           << d.padding.bottom << ", " << d.padding.left << ")";
    }
    if (d.border != EdgeDimensions{}) {
        ss << " border: (" << d.border.top << ", " << d.border.right << ", "
           << d.border.bottom << ", " << d.border.left << ")";
    }
    ss << '\n';

    for (const auto& child : root.children()) {
        ss << dump_layout_tree(*child, depth + 1);
    }

    return ss.str();
}

float document_height(const LayoutBox& root) noexcept {
    float extent = 0.0f;
    auto visit = [&](auto& self, const LayoutBox& box) -> void {
        const auto border_box = box.absolute_border_box();
        extent = std::max(extent, border_box.y + border_box.height + box.dimensions().margin.bottom);
        for (const auto& child : box.children()) {
            self(self, *child);
        }
    };
    visit(visit, root);
    return extent;
}

} // namespace divine::layout
