#pragma once

#include "dom/node.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace divine::css {

struct Specificity {
    uint32_t ids{0};
    uint32_t classes{0};
    uint32_t types{0};

    auto operator<=>(const Specificity&) const = default;

    Specificity& operator+=(const Specificity& other) noexcept {
        ids += other.ids;
        classes += other.classes;
        types += other.types;
        return *this;
    }

    friend Specificity operator+(Specificity lhs, const Specificity& rhs) noexcept {
        lhs += rhs;
        return lhs;
    }
};

// Represents a simple compound selector: e.g. div, .card, #header, or div.card#header
struct SimpleSelector {
    std::string type_name{};
    std::string id{};
    std::vector<std::string> classes{};

    [[nodiscard]] bool matches(const dom::Element& elem) const;
    [[nodiscard]] Specificity specificity() const noexcept;
    [[nodiscard]] bool empty() const noexcept {
        return type_name.empty() && id.empty() && classes.empty();
    }
};

// Represents a complex selector with descendant combinators (e.g. "article .card p")
struct Selector {
    // Left-to-right sequence of components: e.g. [article, .card, p]
    // The last element is the subject selector
    std::vector<SimpleSelector> components{};

    [[nodiscard]] bool matches(const dom::Element& elem) const;
    [[nodiscard]] Specificity specificity() const noexcept;
};

struct Declaration {
    std::string property{};
    std::string value{};

    bool operator==(const Declaration& other) const = default;
};

struct StyleRule {
    std::vector<Selector> selectors{};
    std::vector<Declaration> declarations{};
    size_t source_order{0};
};

struct Stylesheet {
    std::vector<StyleRule> rules{};
};

} // namespace divine::css
