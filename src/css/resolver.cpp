#include "css/resolver.hpp"
#include "css/parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <unordered_set>

namespace divine::css {

namespace {

std::string to_lower_ascii(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

std::vector<std::string_view> split_whitespace(std::string_view str) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    while (start < str.size()) {
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
            ++start;
        }
        if (start >= str.size()) break;
        size_t end = start;
        while (end < str.size() && !std::isspace(static_cast<unsigned char>(str[end]))) {
            ++end;
        }
        tokens.push_back(str.substr(start, end - start));
        start = end;
    }
    return tokens;
}

void parse_edges_shorthand(std::string_view value, BoxEdges& edges) {
    auto tokens = split_whitespace(value);
    if (tokens.empty()) return;

    std::vector<Length> lengths;
    for (auto tok : tokens) {
        auto l = Length::parse(tok);
        if (l) {
            lengths.push_back(*l);
        } else {
            return; // Invalid token
        }
    }

    if (lengths.size() == 1) {
        edges.top = edges.right = edges.bottom = edges.left = lengths[0];
    } else if (lengths.size() == 2) {
        edges.top = edges.bottom = lengths[0];
        edges.right = edges.left = lengths[1];
    } else if (lengths.size() == 3) {
        edges.top = lengths[0];
        edges.right = edges.left = lengths[1];
        edges.bottom = lengths[2];
    } else if (lengths.size() >= 4) {
        edges.top = lengths[0];
        edges.right = lengths[1];
        edges.bottom = lengths[2];
        edges.left = lengths[3];
    }
}

bool is_block_element(std::string_view tag) {
    static constexpr std::array<std::string_view, 24> block_tags = {
        "html", "body", "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
        "header", "footer", "main", "section", "article", "nav", "aside",
        "ul", "ol", "li", "blockquote", "hr", "figure", "figcaption"
    };
    return std::any_of(block_tags.begin(), block_tags.end(),
        [tag](std::string_view b) { return tag == b; });
}

bool is_hidden_element(std::string_view tag) {
    static constexpr std::array<std::string_view, 6> hidden_tags = {
        "head", "title", "meta", "link", "style", "script"
    };
    return std::any_of(hidden_tags.begin(), hidden_tags.end(),
        [tag](std::string_view h) { return tag == h; });
}

struct MatchedDeclaration {
    std::string property;
    std::string value;
    bool is_inline{false};
    Specificity specificity{};
    size_t source_order{0};

    // Higher priority comes first
    bool operator<(const MatchedDeclaration& other) const noexcept {
        if (is_inline != other.is_inline) {
            return is_inline > other.is_inline; // inline declarations win
        }
        if (specificity != other.specificity) {
            return specificity > other.specificity; // higher specificity wins
        }
        return source_order > other.source_order; // later rule in source wins
    }
};

} // namespace

StyleResolver::StyleResolver() = default;

StyleResolver::StyleResolver(Stylesheet stylesheet) : m_stylesheet(std::move(stylesheet)) {}

ComputedStyle StyleResolver::initial_style_for_element(const dom::Element& element) const {
    ComputedStyle style{};
    const std::string tag = to_lower_ascii(element.tag_name());

    if (is_hidden_element(tag)) {
        style.display = Display::None;
    } else if (is_block_element(tag)) {
        style.display = Display::Block;
    } else {
        style.display = Display::Inline;
    }

    if (tag == "h1") {
        style.font_size = 32.0f;
        style.font_weight = 700;
    } else if (tag == "h2") {
        style.font_size = 24.0f;
        style.font_weight = 700;
    } else if (tag == "h3") {
        style.font_size = 18.72f;
        style.font_weight = 700;
    } else if (tag == "h4") {
        style.font_size = 16.0f;
        style.font_weight = 700;
    } else if (tag == "strong" || tag == "b") {
        style.font_weight = 700;
    }

    return style;
}

ComputedStyle StyleResolver::resolve_element_style(
    const dom::Element& element,
    const ComputedStyle* parent_style) const {

    ComputedStyle style = initial_style_for_element(element);
    std::vector<MatchedDeclaration> matched;

    // 1. Collect declarations from matching stylesheet rules
    for (const auto& rule : m_stylesheet.rules) {
        Specificity highest_match_spec{};
        bool matched_any = false;

        for (const auto& sel : rule.selectors) {
            if (sel.matches(element)) {
                auto spec = sel.specificity();
                if (!matched_any || spec > highest_match_spec) {
                    highest_match_spec = spec;
                    matched_any = true;
                }
            }
        }

        if (matched_any) {
            for (const auto& decl : rule.declarations) {
                matched.push_back(MatchedDeclaration{
                    .property = decl.property,
                    .value = decl.value,
                    .is_inline = false,
                    .specificity = highest_match_spec,
                    .source_order = rule.source_order
                });
            }
        }
    }

    // 2. Collect inline styles if present
    if (element.has_attribute("style")) {
        auto inline_attr = element.get_attribute("style").value();
        auto inline_decls = Parser::parse_inline_style(inline_attr);
        for (auto& decl : inline_decls) {
            matched.push_back(MatchedDeclaration{
                .property = std::move(decl.property),
                .value = std::move(decl.value),
                .is_inline = true,
                .specificity = Specificity{},
                .source_order = 0
            });
        }
    }

    // 3. Sort declarations by priority (inline > specificity > source order)
    std::stable_sort(matched.begin(), matched.end());

    // 4. Apply declarations in priority order (first applied wins)
    std::unordered_set<std::string> applied_properties;

    for (const auto& item : matched) {
        if (applied_properties.contains(item.property)) {
            continue;
        }

        const std::string& prop = item.property;
        const std::string& val = item.value;
        const std::string lower_val = to_lower_ascii(val);

        if (prop == "color") {
            auto c = Color::parse(val);
            if (c) {
                style.color = *c;
                applied_properties.insert(prop);
            }
        } else if (prop == "background-color") {
            auto c = Color::parse(val);
            if (c) {
                style.background_color = *c;
                applied_properties.insert(prop);
            }
        } else if (prop == "width") {
            auto l = Length::parse(val);
            if (l) {
                style.width = *l;
                applied_properties.insert(prop);
            }
        } else if (prop == "height") {
            auto l = Length::parse(val);
            if (l) {
                style.height = *l;
                applied_properties.insert(prop);
            }
        } else if (prop == "display") {
            if (lower_val == "block") {
                style.display = Display::Block;
                applied_properties.insert(prop);
            } else if (lower_val == "inline") {
                style.display = Display::Inline;
                applied_properties.insert(prop);
            } else if (lower_val == "none") {
                style.display = Display::None;
                applied_properties.insert(prop);
            }
        } else if (prop == "font-size") {
            auto l = Length::parse(val);
            if (l && l->is_px()) {
                style.font_size = l->value;
                applied_properties.insert(prop);
            }
        } else if (prop == "font-weight") {
            if (lower_val == "bold" || lower_val == "bolder") {
                style.font_weight = 700;
                applied_properties.insert(prop);
            } else if (lower_val == "normal") {
                style.font_weight = 400;
                applied_properties.insert(prop);
            } else if (lower_val == "lighter") {
                style.font_weight = 300;
                applied_properties.insert(prop);
            } else {
                char* end = nullptr;
                unsigned long num = std::strtoul(val.c_str(), &end, 10);
                if (end != val.c_str() && num >= 100 && num <= 900) {
                    style.font_weight = static_cast<uint16_t>(num);
                    applied_properties.insert(prop);
                }
            }
        } else if (prop == "text-align") {
            if (lower_val == "left") {
                style.text_align = TextAlign::Left;
                applied_properties.insert(prop);
            } else if (lower_val == "right") {
                style.text_align = TextAlign::Right;
                applied_properties.insert(prop);
            } else if (lower_val == "center") {
                style.text_align = TextAlign::Center;
                applied_properties.insert(prop);
            } else if (lower_val == "justify") {
                style.text_align = TextAlign::Justify;
                applied_properties.insert(prop);
            }
        } else if (prop == "margin") {
            parse_edges_shorthand(val, style.margin);
            applied_properties.insert(prop);
        } else if (prop == "margin-top") {
            auto l = Length::parse(val);
            if (l) { style.margin.top = *l; applied_properties.insert(prop); }
        } else if (prop == "margin-right") {
            auto l = Length::parse(val);
            if (l) { style.margin.right = *l; applied_properties.insert(prop); }
        } else if (prop == "margin-bottom") {
            auto l = Length::parse(val);
            if (l) { style.margin.bottom = *l; applied_properties.insert(prop); }
        } else if (prop == "margin-left") {
            auto l = Length::parse(val);
            if (l) { style.margin.left = *l; applied_properties.insert(prop); }
        } else if (prop == "padding") {
            parse_edges_shorthand(val, style.padding);
            applied_properties.insert(prop);
        } else if (prop == "padding-top") {
            auto l = Length::parse(val);
            if (l) { style.padding.top = *l; applied_properties.insert(prop); }
        } else if (prop == "padding-right") {
            auto l = Length::parse(val);
            if (l) { style.padding.right = *l; applied_properties.insert(prop); }
        } else if (prop == "padding-bottom") {
            auto l = Length::parse(val);
            if (l) { style.padding.bottom = *l; applied_properties.insert(prop); }
        } else if (prop == "padding-left") {
            auto l = Length::parse(val);
            if (l) { style.padding.left = *l; applied_properties.insert(prop); }
        } else if (prop == "border-width") {
            parse_edges_shorthand(val, style.border_width);
            applied_properties.insert(prop);
        } else if (prop == "border-top-width") {
            auto l = Length::parse(val);
            if (l) { style.border_width.top = *l; applied_properties.insert(prop); }
        } else if (prop == "border-right-width") {
            auto l = Length::parse(val);
            if (l) { style.border_width.right = *l; applied_properties.insert(prop); }
        } else if (prop == "border-bottom-width") {
            auto l = Length::parse(val);
            if (l) { style.border_width.bottom = *l; applied_properties.insert(prop); }
        } else if (prop == "border-left-width") {
            auto l = Length::parse(val);
            if (l) { style.border_width.left = *l; applied_properties.insert(prop); }
        } else if (prop == "border-color") {
            auto color = Color::parse(val);
            if (color) { style.border_color = *color; applied_properties.insert(prop); }
        } else if (prop == "border") {
            auto tokens = split_whitespace(val);
            for (auto tok : tokens) {
                auto l = Length::parse(tok);
                if (l) {
                    style.border_width.top = style.border_width.right =
                    style.border_width.bottom = style.border_width.left = *l;
                } else if (auto color = Color::parse(tok)) {
                    style.border_color = *color;
                }
            }
            const auto functional_color = val.find("rgb(");
            const auto rgba_color = val.find("rgba(");
            const auto color_start = rgba_color != std::string::npos ? rgba_color : functional_color;
            if (color_start != std::string::npos) {
                const auto color_end = val.find(')', color_start);
                if (color_end != std::string::npos) {
                    auto color = Color::parse(std::string_view(val).substr(
                        color_start, color_end - color_start + 1));
                    if (color) style.border_color = *color;
                }
            }
            applied_properties.insert(prop);
        }
    }

    // 5. Apply property inheritance from parent style
    if (parent_style != nullptr) {
        if (!applied_properties.contains("color")) {
            style.color = parent_style->color;
        }
        if (!applied_properties.contains("font-size")) {
            style.font_size = parent_style->font_size;
        }
        if (!applied_properties.contains("font-weight")) {
            style.font_weight = parent_style->font_weight;
        }
        if (!applied_properties.contains("text-align")) {
            style.text_align = parent_style->text_align;
        }
    }

    return style;
}

std::unordered_map<const dom::Element*, ComputedStyle> StyleResolver::resolve_document_styles(
    const dom::Document& doc) const {

    std::unordered_map<const dom::Element*, ComputedStyle> styles;

    auto resolve_recursive = [&](auto& self, const dom::Node& node, const ComputedStyle* parent_style) -> void {
        const ComputedStyle* current_style = parent_style;

        if (node.is_element()) {
            const auto& elem = static_cast<const dom::Element&>(node);
            ComputedStyle computed = resolve_element_style(elem, parent_style);
            auto [it, _] = styles.emplace(&elem, std::move(computed));
            current_style = &it->second;
        }

        for (const auto& child : node.children()) {
            self(self, *child, current_style);
        }
    };

    resolve_recursive(resolve_recursive, doc, nullptr);
    return styles;
}

} // namespace divine::css
