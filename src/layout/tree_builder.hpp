#pragma once

#include "css/types.hpp"
#include "dom/node.hpp"
#include "layout/box.hpp"

#include <memory>
#include <unordered_map>

namespace divine::layout {

class LayoutTreeBuilder {
public:
    static std::unique_ptr<LayoutBox> build(
        const dom::Node& root,
        const std::unordered_map<const dom::Element*, css::ComputedStyle>& styles);

private:
    static std::unique_ptr<LayoutBox> build_recursive(
        const dom::Node& node,
        const std::unordered_map<const dom::Element*, css::ComputedStyle>& styles,
        const css::ComputedStyle* parent_style);
};

} // namespace divine::layout
