#include "layout/tree_builder.hpp"

#include <algorithm>
#include <cctype>

namespace divine::layout {

namespace {

bool is_only_whitespace(std::string_view str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    });
}

} // namespace

std::unique_ptr<LayoutBox> LayoutTreeBuilder::build_recursive(
    const dom::Node& node,
    const std::unordered_map<const dom::Element*, css::ComputedStyle>& styles,
    const css::ComputedStyle* parent_style) {

    if (node.is_element()) {
        const auto& elem = static_cast<const dom::Element&>(node);
        auto it = styles.find(&elem);
        const css::ComputedStyle style = (it != styles.end()) ? it->second : css::ComputedStyle{};

        if (style.display == css::Display::None) {
            return nullptr;
        }

        const BoxType box_type = (style.display == css::Display::Block)
            ? BoxType::Block
            : BoxType::Inline;

        auto box = std::make_unique<LayoutBox>(box_type, &node, &style);

        // Check if element has any block children
        bool has_block_child = false;
        for (const auto& child : node.children()) {
            if (child->is_element()) {
                const auto& child_elem = static_cast<const dom::Element&>(*child);
                auto cit = styles.find(&child_elem);
                if (cit != styles.end() && cit->second.display == css::Display::Block) {
                    has_block_child = true;
                    break;
                }
            }
        }

        for (const auto& child : node.children()) {
            if (child->is_text() && has_block_child) {
                const auto& txt = static_cast<const dom::TextNode&>(*child);
                if (is_only_whitespace(txt.data())) {
                    continue; // Skip inter-element formatting whitespace in block contexts
                }
            }

            auto child_box = build_recursive(*child, styles, &style);
            if (child_box) {
                box->append_child(std::move(child_box));
            }
        }

        return box;
    }

    if (node.is_text()) {
        const auto& txt = static_cast<const dom::TextNode&>(node);
        if (txt.data().empty()) {
            return nullptr;
        }

        auto box = std::make_unique<LayoutBox>(BoxType::Text, &node, parent_style);
        box->set_text(txt.data());
        return box;
    }

    return nullptr;
}

std::unique_ptr<LayoutBox> LayoutTreeBuilder::build(
    const dom::Node& root,
    const std::unordered_map<const dom::Element*, css::ComputedStyle>& styles) {

    if (root.is_document()) {
        const auto& doc = static_cast<const dom::Document&>(root);
        auto root_box = std::make_unique<LayoutBox>(BoxType::Anonymous, &root, nullptr);
        for (const auto& child : root.children()) {
            auto child_box = build_recursive(*child, styles, nullptr);
            if (child_box) {
                root_box->append_child(std::move(child_box));
            }
        }
        return root_box;
    }

    return build_recursive(root, styles, nullptr);
}

} // namespace divine::layout
