#pragma once

#include "css/stylesheet.hpp"
#include "css/types.hpp"
#include "dom/node.hpp"

#include <unordered_map>

namespace divine::css {

class StyleResolver {
public:
    StyleResolver();
    explicit StyleResolver(Stylesheet stylesheet);

    void set_stylesheet(Stylesheet stylesheet) { m_stylesheet = std::move(stylesheet); }
    [[nodiscard]] const Stylesheet& stylesheet() const noexcept { return m_stylesheet; }

    [[nodiscard]] ComputedStyle resolve_element_style(
        const dom::Element& element,
        const ComputedStyle* parent_style = nullptr) const;

    [[nodiscard]] std::unordered_map<const dom::Element*, ComputedStyle> resolve_document_styles(
        const dom::Document& doc) const;

private:
    [[nodiscard]] ComputedStyle initial_style_for_element(const dom::Element& element) const;

    Stylesheet m_stylesheet{};
};

} // namespace divine::css
