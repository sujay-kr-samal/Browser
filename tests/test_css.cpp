#include "css/parser.hpp"
#include "css/resolver.hpp"
#include "css/types.hpp"
#include "html/parser.hpp"

#include <cassert>
#include <iostream>

using namespace divine;

void test_color_and_length_parsing() {
    // Named colors
    auto c_black = css::Color::parse("black");
    assert(c_black.has_value() && *c_black == css::Color::black());

    auto c_trans = css::Color::parse("transparent");
    assert(c_trans.has_value() && *c_trans == css::Color::transparent());

    // Hex colors
    auto c_hex3 = css::Color::parse("#f0a");
    assert(c_hex3.has_value() && c_hex3->r == 255 && c_hex3->g == 0 && c_hex3->b == 170 && c_hex3->a == 255);

    auto c_hex6 = css::Color::parse("#1a73e8");
    assert(c_hex6.has_value() && c_hex6->r == 0x1a && c_hex6->g == 0x73 && c_hex6->b == 0xe8 && c_hex6->a == 255);

    auto c_hex8 = css::Color::parse("#11223344");
    assert(c_hex8.has_value() && c_hex8->r == 0x11 && c_hex8->g == 0x22 && c_hex8->b == 0x33 && c_hex8->a == 0x44);

    // RGB & RGBA
    auto c_rgb = css::Color::parse("rgb(50, 100, 150)");
    assert(c_rgb.has_value() && c_rgb->r == 50 && c_rgb->g == 100 && c_rgb->b == 150 && c_rgb->a == 255);

    auto c_rgba = css::Color::parse("rgba(50, 100, 150, 0.5)");
    assert(c_rgba.has_value() && c_rgba->r == 50 && c_rgba->g == 100 && c_rgba->b == 150 && c_rgba->a == 127);

    // Lengths
    auto l_px = css::Length::parse("42px");
    assert(l_px.has_value() && l_px->is_px() && l_px->value == 42.0f);

    auto l_zero = css::Length::parse("0");
    assert(l_zero.has_value() && l_zero->is_px() && l_zero->value == 0.0f);

    auto l_auto = css::Length::parse("auto");
    assert(l_auto.has_value() && l_auto->is_auto());

    std::cout << "[PASS] test_color_and_length_parsing\n";
}

void test_css_parsing() {
    const std::string css_source =
        "/* Header comment */\n"
        "h1, h2, h3 {\n"
        "    color: #1a73e8;\n"
        "    font-weight: bold;\n"
        "    margin: 10px 20px;\n"
        "}\n"
        ".card {\n"
        "    background-color: white;\n"
        "    padding: 15px;\n"
        "    display: block;\n"
        "}\n"
        "#content {\n"
        "    width: 800px;\n"
        "    height: auto;\n"
        "}\n";

    auto sheet = css::Parser::parse(css_source);
    assert(sheet.rules.size() == 3);

    // Rule 0
    const auto& r0 = sheet.rules[0];
    assert(r0.selectors.size() == 3);
    assert(r0.selectors[0].components[0].type_name == "h1");
    assert(r0.selectors[1].components[0].type_name == "h2");
    assert(r0.selectors[2].components[0].type_name == "h3");
    assert(r0.declarations.size() == 3);
    assert(r0.declarations[0].property == "color" && r0.declarations[0].value == "#1a73e8");
    assert(r0.declarations[1].property == "font-weight" && r0.declarations[1].value == "bold");
    assert(r0.declarations[2].property == "margin" && r0.declarations[2].value == "10px 20px");

    // Rule 1
    const auto& r1 = sheet.rules[1];
    assert(r1.selectors.size() == 1);
    assert(r1.selectors[0].components[0].classes.size() == 1);
    assert(r1.selectors[0].components[0].classes[0] == "card");
    assert(r1.declarations.size() == 3);

    // Rule 2
    const auto& r2 = sheet.rules[2];
    assert(r2.selectors.size() == 1);
    assert(r2.selectors[0].components[0].id == "content");
    assert(r2.declarations.size() == 2);

    std::cout << "[PASS] test_css_parsing\n";
}

void test_selector_matching() {
    const std::string html =
        "<div id=\"container\" class=\"wrapper dark-mode\">\n"
        "  <article class=\"post\">\n"
        "    <h1 class=\"title\">Heading</h1>\n"
        "    <p id=\"lead-paragraph\" class=\"lead text-bold\">First paragraph</p>\n"
        "    <p>Second paragraph</p>\n"
        "  </article>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    assert(doc != nullptr);

    auto* container = doc->get_element_by_id("container");
    auto* lead_p = doc->get_element_by_id("lead-paragraph");
    auto p_list = doc->get_elements_by_tag_name("p");
    assert(container != nullptr && lead_p != nullptr && p_list.size() == 2);

    // 1. Type selector
    auto sel_p = *css::Parser::parse_single_selector("p");
    assert(sel_p.matches(*lead_p));
    assert(sel_p.matches(*p_list[1]));
    assert(!sel_p.matches(*container));

    // 2. Class selector
    auto sel_lead = *css::Parser::parse_single_selector(".lead");
    assert(sel_lead.matches(*lead_p));
    assert(!sel_lead.matches(*p_list[1]));

    // 3. ID selector
    auto sel_id = *css::Parser::parse_single_selector("#lead-paragraph");
    assert(sel_id.matches(*lead_p));
    assert(!sel_id.matches(*p_list[1]));

    // 4. Compound selector
    auto sel_compound = *css::Parser::parse_single_selector("div.wrapper#container");
    assert(sel_compound.matches(*container));
    assert(!sel_compound.matches(*lead_p));

    // 5. Descendant selector (article p)
    auto sel_desc = *css::Parser::parse_single_selector("article p");
    assert(sel_desc.matches(*lead_p));
    assert(sel_desc.matches(*p_list[1]));
    assert(!sel_desc.matches(*container));

    // 6. Multi-level descendant selector (#container .post .lead)
    auto sel_multi_desc = *css::Parser::parse_single_selector("#container .post .lead");
    assert(sel_multi_desc.matches(*lead_p));
    assert(!sel_multi_desc.matches(*p_list[1]));

    // 7. Non-matching descendant selector (section p)
    auto sel_nonmatch = *css::Parser::parse_single_selector("section p");
    assert(!sel_nonmatch.matches(*lead_p));

    std::cout << "[PASS] test_selector_matching\n";
}

void test_specificity_and_precedence() {
    auto get_spec = [](std::string_view css_str) {
        return css::Parser::parse_single_selector(css_str)->specificity();
    };

    auto spec_type = get_spec("p");
    assert(spec_type.ids == 0 && spec_type.classes == 0 && spec_type.types == 1);

    auto spec_class = get_spec(".card");
    assert(spec_class.ids == 0 && spec_class.classes == 1 && spec_class.types == 0);

    auto spec_id = get_spec("#main");
    assert(spec_id.ids == 1 && spec_id.classes == 0 && spec_id.types == 0);

    auto spec_desc = get_spec("article .card p");
    assert(spec_desc.ids == 0 && spec_desc.classes == 1 && spec_desc.types == 2);

    auto spec_complex = get_spec("div#main .card.active p");
    assert(spec_complex.ids == 1 && spec_complex.classes == 2 && spec_complex.types == 2);

    // Verify ordering
    assert(spec_type < spec_class);
    assert(spec_class < spec_id);
    assert(spec_desc < spec_id);
    assert(spec_id < spec_complex);

    // Style conflict resolution test
    const std::string css_code =
        "p { color: blue; }\n"
        ".highlight { color: green; }\n"
        "#urgent { color: red; }\n";

    auto doc = html::Parser::parse_string("<p id=\"urgent\" class=\"highlight\">Conflict Test</p>");
    auto* p = doc->document_element();
    assert(p != nullptr);

    css::StyleResolver resolver(css::Parser::parse(css_code));
    auto style = resolver.resolve_element_style(*p);
    // #urgent has highest specificity, should be red
    assert(style.color == *css::Color::parse("red"));

    // Source-order tie-breaking
    const std::string tie_css =
        ".box { background-color: yellow; }\n"
        ".box { background-color: purple; }\n";

    auto doc_box = html::Parser::parse_string("<div class=\"box\">Tie test</div>");
    auto* div_box = doc_box->document_element();
    css::StyleResolver tie_resolver(css::Parser::parse(tie_css));
    auto box_style = tie_resolver.resolve_element_style(*div_box);
    // Later rule in source wins
    assert(box_style.background_color == *css::Color::parse("purple"));

    // Inline style override
    auto doc_inline = html::Parser::parse_string("<p id=\"urgent\" style=\"color: yellow;\">Inline</p>");
    auto* p_inline = doc_inline->document_element();
    auto inline_style = resolver.resolve_element_style(*p_inline);
    // Inline style overrides ID selector
    assert(inline_style.color == *css::Color::parse("yellow"));

    std::cout << "[PASS] test_specificity_and_precedence\n";
}

void test_inheritance_and_defaults() {
    const std::string html =
        "<div style=\"color: blue; font-size: 24px; font-weight: bold; text-align: center;\">\n"
        "  <p id=\"inherited-p\">Inherited</p>\n"
        "  <span id=\"custom-span\" style=\"color: red;\">Custom color</span>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto* p = doc->get_element_by_id("inherited-p");
    auto* span = doc->get_element_by_id("custom-span");
    assert(p != nullptr && span != nullptr);

    const auto& p_style = styles.at(p);
    const auto& span_style = styles.at(span);

    // p inherits color, font-size, font-weight, text-align from div
    assert(p_style.color == *css::Color::parse("blue"));
    assert(p_style.font_size == 24.0f);
    assert(p_style.font_weight == 700);
    assert(p_style.text_align == css::TextAlign::Center);
    assert(p_style.display == css::Display::Block); // p defaults to block

    // span overrides color, but inherits font-size, font-weight, text-align
    assert(span_style.color == *css::Color::parse("red"));
    assert(span_style.font_size == 24.0f);
    assert(span_style.font_weight == 700);
    assert(span_style.text_align == css::TextAlign::Center);
    assert(span_style.display == css::Display::Inline); // span defaults to inline

    std::cout << "[PASS] test_inheritance_and_defaults\n";
}

void test_box_edges_shorthand() {
    const std::string css_code =
        ".b1 { margin: 10px; }\n"
        ".b2 { margin: 10px 20px; }\n"
        ".b3 { margin: 10px 20px 30px; }\n"
        ".b4 { margin: 10px 20px 30px 40px; }\n";

    auto sheet = css::Parser::parse(css_code);
    css::StyleResolver resolver(sheet);

    auto doc = html::Parser::parse_string(
        "<div class=\"b1\"></div><div class=\"b2\"></div><div class=\"b3\"></div><div class=\"b4\"></div>");
    auto elements = doc->get_elements_by_tag_name("div");
    assert(elements.size() == 4);

    // 1-value shorthand
    auto s1 = resolver.resolve_element_style(*elements[0]);
    assert(s1.margin.top.value == 10.0f && s1.margin.right.value == 10.0f &&
           s1.margin.bottom.value == 10.0f && s1.margin.left.value == 10.0f);

    // 2-value shorthand
    auto s2 = resolver.resolve_element_style(*elements[1]);
    assert(s2.margin.top.value == 10.0f && s2.margin.right.value == 20.0f &&
           s2.margin.bottom.value == 10.0f && s2.margin.left.value == 20.0f);

    // 3-value shorthand
    auto s3 = resolver.resolve_element_style(*elements[2]);
    assert(s3.margin.top.value == 10.0f && s3.margin.right.value == 20.0f &&
           s3.margin.bottom.value == 30.0f && s3.margin.left.value == 20.0f);

    // 4-value shorthand
    auto s4 = resolver.resolve_element_style(*elements[3]);
    assert(s4.margin.top.value == 10.0f && s4.margin.right.value == 20.0f &&
           s4.margin.bottom.value == 30.0f && s4.margin.left.value == 40.0f);

    std::cout << "[PASS] test_box_edges_shorthand\n";
}

void test_malformed_css_resilience() {
    // 1. Invalid declarations within a rule
    {
        const std::string css_code =
            "p {\n"
            "    color: ;\n"
            "    font-size: 22px;\n"
            "    : no-property;\n"
            "    width: 300px;\n"
            "}\n";
        auto sheet = css::Parser::parse(css_code);
        assert(sheet.rules.size() == 1);
        auto doc = html::Parser::parse_string("<p>Text</p>");
        css::StyleResolver resolver(sheet);
        auto style = resolver.resolve_element_style(*doc->document_element());
        assert(style.font_size == 22.0f);
        assert(style.width == css::Length::px(300.0f));
    }

    // 2. Syntax recovery between rules
    {
        const std::string css_code =
            "h1 { color: red; }\n"
            "bad selector without block\n"
            "@media screen { p { color: green; } }\n"
            "h2 { color: blue; }\n";
        auto sheet = css::Parser::parse(css_code);
        assert(sheet.rules.size() >= 2);
    }

    // 3. Unclosed comment and unclosed block at EOF
    {
        auto s1 = css::Parser::parse("/* unclosed comment at EOF");
        assert(s1.rules.empty());

        auto s2 = css::Parser::parse("p { color: red");
        assert(!s2.rules.empty());
    }

    std::cout << "[PASS] test_malformed_css_resilience\n";
}

int main() {
    std::cout << "Running Divine Engine CSS & Style Resolution Tests...\n";
    test_color_and_length_parsing();
    test_css_parsing();
    test_selector_matching();
    test_specificity_and_precedence();
    test_inheritance_and_defaults();
    test_box_edges_shorthand();
    test_malformed_css_resilience();
    std::cout << "All CSS & Style Resolution Tests Passed Successfully!\n";
    return 0;
}
