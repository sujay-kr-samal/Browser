#include "css/parser.hpp"
#include "css/resolver.hpp"
#include "html/parser.hpp"
#include "layout/box.hpp"
#include "layout/tree_builder.hpp"

#include <cassert>
#include <iostream>

using namespace divine;

void test_single_element_geometry() {
    const std::string html =
        "<div style=\"width: 200px; height: 100px; padding: 20px; margin: 10px; border-width: 5px;\"></div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto layout_root = layout::LayoutTreeBuilder::build(*doc, styles);
    assert(layout_root != nullptr);

    layout::Dimensions cb;
    cb.content.width = 800.0f;
    layout_root->layout(cb);

    const auto& d = layout_root->dimensions();

    // Content box: width=200, height=100
    assert(d.content.width == 200.0f);
    assert(d.content.height == 100.0f);

    // Padding: 20 all sides
    assert(d.padding.top == 20.0f && d.padding.right == 20.0f &&
           d.padding.bottom == 20.0f && d.padding.left == 20.0f);

    // Border: 5 all sides
    assert(d.border.top == 5.0f && d.border.right == 5.0f &&
           d.border.bottom == 5.0f && d.border.left == 5.0f);

    // Margin: 10 all sides
    assert(d.margin.top == 10.0f && d.margin.right == 10.0f &&
           d.margin.bottom == 10.0f && d.margin.left == 10.0f);

    // Content position: margin.left(10) + border.left(5) + padding.left(20) = 35
    assert(d.content.x == 35.0f);
    assert(d.content.y == 35.0f);

    // Box model geometry helper calculations
    auto p_box = d.padding_box();
    assert(p_box.x == 15.0f && p_box.y == 15.0f);
    assert(p_box.width == 240.0f && p_box.height == 140.0f);

    auto b_box = d.border_box();
    assert(b_box.x == 10.0f && b_box.y == 10.0f);
    assert(b_box.width == 250.0f && b_box.height == 150.0f);

    auto m_box = d.margin_box();
    assert(m_box.x == 0.0f && m_box.y == 0.0f);
    assert(m_box.width == 270.0f && m_box.height == 170.0f);

    std::cout << "[PASS] test_single_element_geometry\n";
}

void test_nested_block_elements() {
    const std::string html =
        "<div id=\"parent\" style=\"width: 500px; padding: 25px;\">\n"
        "  <div id=\"child\" style=\"width: auto; height: 50px; margin: 15px;\"></div>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto layout_root = layout::LayoutTreeBuilder::build(*doc, styles);
    assert(layout_root != nullptr);

    layout::Dimensions cb;
    cb.content.width = 1000.0f;
    layout_root->layout(cb);

    // Parent
    const auto& pd = layout_root->dimensions();
    assert(pd.content.width == 500.0f);
    assert(pd.padding.left == 25.0f && pd.padding.right == 25.0f);

    // Child
    assert(layout_root->children().size() == 1);
    const auto* child = layout_root->children()[0].get();
    const auto& cd = child->dimensions();

    // Child width = parent content width (500) - child margin (15+15) = 470
    assert(cd.content.width == 470.0f);
    assert(cd.content.height == 50.0f);
    assert(cd.content.x == 15.0f);
    assert(cd.content.y == 15.0f);

    // Parent auto-height = child margin-box height = 50 + 15 + 15 = 80
    assert(pd.content.height == 80.0f);

    std::cout << "[PASS] test_nested_block_elements\n";
}

void test_vertical_stacking() {
    const std::string html =
        "<div id=\"container\" style=\"width: 600px;\">\n"
        "  <div id=\"b1\" style=\"height: 40px; margin-bottom: 10px;\"></div>\n"
        "  <div id=\"b2\" style=\"height: 60px; margin-top: 15px; margin-bottom: 5px;\"></div>\n"
        "  <div id=\"b3\" style=\"height: 80px;\"></div>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto container = layout::LayoutTreeBuilder::build(*doc, styles);
    assert(container != nullptr);

    layout::Dimensions cb;
    cb.content.width = 800.0f;
    container->layout(cb);

    assert(container->children().size() == 3);
    const auto* b1 = container->children()[0].get();
    const auto* b2 = container->children()[1].get();
    const auto* b3 = container->children()[2].get();

    // b1: y = 0, height = 40, margin_box height = 40 + 10 = 50
    assert(b1->dimensions().content.y == 0.0f);
    assert(b1->dimensions().content.height == 40.0f);
    assert(b1->dimensions().margin_box().height == 50.0f);

    // b2: y = 50 + 15 = 65, height = 60, margin_box height = 15 + 60 + 5 = 80
    assert(b2->dimensions().content.y == 65.0f);
    assert(b2->dimensions().content.height == 60.0f);
    assert(b2->dimensions().margin_box().height == 80.0f);

    // b3: y = 50 + 80 = 130, height = 80, margin_box height = 80
    assert(b3->dimensions().content.y == 130.0f);
    assert(b3->dimensions().content.height == 80.0f);
    assert(b3->dimensions().margin_box().height == 80.0f);

    // Container auto-height = 50 + 80 + 80 = 210
    assert(container->dimensions().content.height == 210.0f);

    std::cout << "[PASS] test_vertical_stacking\n";
}

void test_parent_relative_and_absolute_coordinates() {
    const std::string html =
        "<div id=\"outer\" style=\"width: 600px; margin: 30px; padding: 20px;\">\n"
        "  <div id=\"inner\" style=\"width: 200px; height: 100px; margin: 10px;\"></div>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto outer = layout::LayoutTreeBuilder::build(*doc, styles);
    assert(outer != nullptr);

    layout::Dimensions cb;
    cb.content.width = 1000.0f;
    outer->layout(cb);

    const auto* inner = outer->children()[0].get();

    // Outer content origin: margin.left(30) + padding.left(20) = 50
    assert(outer->dimensions().content.x == 50.0f);
    assert(outer->dimensions().content.y == 50.0f);

    // Inner parent-relative coordinate: margin.left(10) = 10
    assert(inner->dimensions().content.x == 10.0f);
    assert(inner->dimensions().content.y == 10.0f);

    // Inner absolute coordinates in viewport: 50 + 10 = 60
    auto abs_content = inner->absolute_content_box();
    assert(abs_content.x == 60.0f);
    assert(abs_content.y == 60.0f);
    assert(abs_content.width == 200.0f);
    assert(abs_content.height == 100.0f);

    std::cout << "[PASS] test_parent_relative_and_absolute_coordinates\n";
}

void test_auto_margins_centering() {
    const std::string html =
        "<div id=\"parent\" style=\"width: 600px;\">\n"
        "  <div id=\"centered\" style=\"width: 200px; height: 50px; margin: 0 auto;\"></div>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto parent = layout::LayoutTreeBuilder::build(*doc, styles);
    layout::Dimensions cb;
    cb.content.width = 800.0f;
    parent->layout(cb);

    const auto* centered = parent->children()[0].get();
    const auto& d = centered->dimensions();

    // Remaining space = 600 - 200 = 400; auto margin.left = margin.right = 200
    assert(d.margin.left == 200.0f);
    assert(d.margin.right == 200.0f);
    assert(d.content.x == 200.0f);
    assert(d.content.width == 200.0f);

    std::cout << "[PASS] test_auto_margins_centering\n";
}

void test_empty_elements() {
    const std::string html = "<div id=\"empty\" style=\"width: 400px; margin: 10px;\"></div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto box = layout::LayoutTreeBuilder::build(*doc, styles);
    layout::Dimensions cb;
    cb.content.width = 800.0f;
    box->layout(cb);

    // Auto-height on empty block box with no children is 0
    assert(box->dimensions().content.width == 400.0f);
    assert(box->dimensions().content.height == 0.0f);
    assert(box->dimensions().margin_box().height == 20.0f); // 10 top + 10 bottom

    std::cout << "[PASS] test_empty_elements\n";
}

void test_viewport_resizing() {
    const std::string html =
        "<div id=\"root\" style=\"width: auto; padding: 20px;\">\n"
        "  <p style=\"width: auto; height: 40px;\"></p>\n"
        "</div>";

    auto doc = html::Parser::parse_string(html);
    css::StyleResolver resolver;
    auto styles = resolver.resolve_document_styles(*doc);

    auto box = layout::LayoutTreeBuilder::build(*doc, styles);

    // 1. Layout with viewport width 1000px
    layout::Dimensions cb1000;
    cb1000.content.width = 1000.0f;
    box->layout(cb1000);

    // Root content width = 1000 - 20*2 = 960
    assert(box->dimensions().content.width == 960.0f);
    assert(box->children()[0]->dimensions().content.width == 960.0f);

    // 2. Resize viewport to 600px
    layout::Dimensions cb600;
    cb600.content.width = 600.0f;
    box->layout(cb600);

    // Root content width dynamically adapts to 600 - 20*2 = 560
    assert(box->dimensions().content.width == 560.0f);
    assert(box->children()[0]->dimensions().content.width == 560.0f);

    std::cout << "[PASS] test_viewport_resizing\n";
}

int main() {
    std::cout << "Running Divine Engine Layout Tests...\n";
    test_single_element_geometry();
    test_nested_block_elements();
    test_vertical_stacking();
    test_parent_relative_and_absolute_coordinates();
    test_auto_margins_centering();
    test_empty_elements();
    test_viewport_resizing();
    std::cout << "All Divine Engine Layout Tests Passed Successfully!\n";
    return 0;
}
