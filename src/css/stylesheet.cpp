#include "css/stylesheet.hpp"

#include <cctype>

namespace divine::css {

namespace {

bool element_has_class(const dom::Element& elem, std::string_view target_class) {
    std::string_view class_attr = elem.class_name();
    size_t start = 0;
    while (start < class_attr.size()) {
        while (start < class_attr.size() && std::isspace(static_cast<unsigned char>(class_attr[start]))) {
            ++start;
        }
        if (start >= class_attr.size()) {
            break;
        }
        size_t end = start;
        while (end < class_attr.size() && !std::isspace(static_cast<unsigned char>(class_attr[end]))) {
            ++end;
        }
        if (class_attr.substr(start, end - start) == target_class) {
            return true;
        }
        start = end;
    }
    return false;
}

bool string_equals_ci(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

bool SimpleSelector::matches(const dom::Element& elem) const {
    if (!type_name.empty() && type_name != "*") {
        if (!string_equals_ci(elem.tag_name(), type_name)) {
            return false;
        }
    }

    if (!id.empty()) {
        if (elem.id() != id) {
            return false;
        }
    }

    for (const auto& cls : classes) {
        if (!element_has_class(elem, cls)) {
            return false;
        }
    }

    return true;
}

Specificity SimpleSelector::specificity() const noexcept {
    Specificity spec{};
    if (!id.empty()) {
        spec.ids = 1;
    }
    spec.classes = static_cast<uint32_t>(classes.size());
    if (!type_name.empty() && type_name != "*") {
        spec.types = 1;
    }
    return spec;
}

bool Selector::matches(const dom::Element& elem) const {
    if (components.empty()) {
        return false;
    }

    // The rightmost component must match the target element directly
    if (!components.back().matches(elem)) {
        return false;
    }

    if (components.size() == 1) {
        return true;
    }

    // Match ancestors from right to left (descendant combinator)
    size_t comp_idx = components.size() - 2;
    const dom::Node* curr = elem.parent();

    while (curr != nullptr) {
        if (curr->is_element()) {
            const auto& parent_elem = static_cast<const dom::Element&>(*curr);
            if (components[comp_idx].matches(parent_elem)) {
                if (comp_idx == 0) {
                    return true; // Matched all ancestor components
                }
                --comp_idx;
            }
        }
        curr = curr->parent();
    }

    return false;
}

Specificity Selector::specificity() const noexcept {
    Specificity total{};
    for (const auto& comp : components) {
        total += comp.specificity();
    }
    return total;
}

} // namespace divine::css
