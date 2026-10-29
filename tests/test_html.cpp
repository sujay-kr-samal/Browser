#include "dom/node.hpp"
#include "html/parser.hpp"
#include "html/tokenizer.hpp"

#include <cassert>
#include <iostream>
#include <string>

using namespace divine;

void test_empty_document() {
    auto doc = html::Parser::parse_string("");
    assert(doc != nullptr && "Document pointer must not be null");
    assert(doc->document_element() == nullptr && "Empty document should have no document element");
    assert(doc->children().empty() && "Empty document should have no child nodes");

    auto doc_whitespace = html::Parser::parse_string("   \n\t   ");
    assert(doc_whitespace != nullptr);
    assert(doc_whitespace->document_element() == nullptr);

    std::cout << "[PASS] test_empty_document\n";
}

void test_nested_elements() {
    const std::string source = "<div><section><article><p>Nested Content</p></article></section></div>";
    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    dom::Element* div = doc->document_element();
    assert(div != nullptr && div->tag_name() == "div");
    assert(div->parent() == doc.get());

    assert(div->children().size() == 1);
    auto* section = static_cast<dom::Element*>(div->first_child());
    assert(section->is_element() && section->tag_name() == "section");
    assert(section->parent() == div);

    assert(section->children().size() == 1);
    auto* article = static_cast<dom::Element*>(section->first_child());
    assert(article->is_element() && article->tag_name() == "article");
    assert(article->parent() == section);

    assert(article->children().size() == 1);
    auto* p = static_cast<dom::Element*>(article->first_child());
    assert(p->is_element() && p->tag_name() == "p");
    assert(p->parent() == article);

    assert(p->children().size() == 1);
    auto* txt = static_cast<dom::TextNode*>(p->first_child());
    assert(txt->is_text() && txt->data() == "Nested Content");
    assert(txt->parent() == p);

    std::cout << "[PASS] test_nested_elements\n";
}

void test_preserving_text_content() {
    const std::string source = "<p>  Hello,  Divine Engine!  &amp; &lt;Rock &amp; Roll&gt;  </p>";
    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    dom::Element* p = doc->document_element();
    assert(p != nullptr && p->tag_name() == "p");
    assert(p->children().size() == 1);

    auto* text = static_cast<dom::TextNode*>(p->first_child());
    assert(text != nullptr && text->is_text());
    assert(text->data() == "  Hello,  Divine Engine!  & <Rock & Roll>  ");

    std::cout << "[PASS] test_preserving_text_content\n";
}

void test_multiple_attributes() {
    const std::string source =
        "<input id=\"username\" class='form-control input-lg' type=text required disabled data-count=\"99\">";
    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    dom::Element* input = doc->document_element();
    assert(input != nullptr && input->tag_name() == "input");

    assert(input->id() == "username");
    assert(input->class_name() == "form-control input-lg");

    assert(input->has_attribute("type"));
    assert(input->get_attribute("type") == "text");

    assert(input->has_attribute("required"));
    assert(input->has_attribute("disabled"));

    assert(input->has_attribute("data-count"));
    assert(input->get_attribute("data-count") == "99");

    assert(!input->has_attribute("nonexistent"));
    assert(!input->get_attribute("nonexistent").has_value());

    std::cout << "[PASS] test_multiple_attributes\n";
}

void test_comments_and_doctypes() {
    const std::string source =
        "<!DOCTYPE html><!-- Header comment --><html><body><!-- Body comment -->Hello</body></html>";
    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    assert(doc->doctype() == "html");

    // Children of document: doctype, comment, html element
    const auto& root_children = doc->children();
    assert(root_children.size() >= 3);
    assert(root_children[0]->is_doctype());
    assert(static_cast<dom::DocumentTypeNode*>(root_children[0].get())->name() == "html");

    assert(root_children[1]->is_comment());
    assert(static_cast<dom::CommentNode*>(root_children[1].get())->data() == " Header comment ");

    dom::Element* html_elem = doc->document_element();
    assert(html_elem != nullptr && html_elem->tag_name() == "html");

    auto* body = static_cast<dom::Element*>(html_elem->first_child());
    assert(body != nullptr && body->tag_name() == "body");

    assert(body->children().size() == 2);
    assert(body->children()[0]->is_comment());
    assert(static_cast<dom::CommentNode*>(body->children()[0].get())->data() == " Body comment ");
    assert(body->children()[1]->is_text());
    assert(static_cast<dom::TextNode*>(body->children()[1].get())->data() == "Hello");

    std::cout << "[PASS] test_comments_and_doctypes\n";
}

void test_void_and_self_closing_elements() {
    const std::string source =
        "<div>"
        "Line 1<br>"
        "Line 2<hr>"
        "<img src=\"icon.png\" alt=\"app icon\" />"
        "<input type=checkbox checked />"
        "<span>End</span>"
        "</div>";

    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    dom::Element* div = doc->document_element();
    assert(div != nullptr && div->tag_name() == "div");

    // div should contain: Text("Line 1"), Element("br"), Text("Line 2"), Element("hr"),
    // Element("img"), Element("input"), Element("span")
    const auto& children = div->children();
    assert(children.size() == 7);

    assert(children[0]->is_text() && static_cast<dom::TextNode*>(children[0].get())->data() == "Line 1");
    assert(children[1]->is_element() && static_cast<dom::Element*>(children[1].get())->tag_name() == "br");
    assert(children[2]->is_text() && static_cast<dom::TextNode*>(children[2].get())->data() == "Line 2");
    assert(children[3]->is_element() && static_cast<dom::Element*>(children[3].get())->tag_name() == "hr");

    auto* img = static_cast<dom::Element*>(children[4].get());
    assert(img->is_element() && img->tag_name() == "img");
    assert(img->get_attribute("src") == "icon.png");
    assert(img->get_attribute("alt") == "app icon");

    auto* input = static_cast<dom::Element*>(children[5].get());
    assert(input->is_element() && input->tag_name() == "input");
    assert(input->get_attribute("type") == "checkbox");
    assert(input->has_attribute("checked"));

    auto* span = static_cast<dom::Element*>(children[6].get());
    assert(span->is_element() && span->tag_name() == "span");
    assert(span->children().size() == 1);
    assert(static_cast<dom::TextNode*>(span->first_child())->data() == "End");

    std::cout << "[PASS] test_void_and_self_closing_elements\n";
}

void test_malformed_input() {
    // 1. Unclosed paragraph closed by parent div
    {
        auto doc = html::Parser::parse_string("<div><p>Unclosed paragraph</div>");
        assert(doc != nullptr);
        dom::Element* div = doc->document_element();
        assert(div != nullptr && div->tag_name() == "div");
        assert(div->children().size() == 1);
        auto* p = static_cast<dom::Element*>(div->first_child());
        assert(p->tag_name() == "p");
        assert(static_cast<dom::TextNode*>(p->first_child())->data() == "Unclosed paragraph");
    }

    // 2. Unterminated tag at EOF
    {
        auto doc = html::Parser::parse_string("<div id=\"unterminated");
        assert(doc != nullptr);
    }

    // 3. Unterminated comment at EOF
    {
        auto doc = html::Parser::parse_string("<!-- comment with no end");
        assert(doc != nullptr);
        assert(!doc->children().empty());
        assert(doc->children()[0]->is_comment());
    }

    // 4. Stray closing tags
    {
        auto doc = html::Parser::parse_string("</div></span></p><p>Valid</p>");
        assert(doc != nullptr);
        auto* p = doc->document_element();
        assert(p != nullptr && p->tag_name() == "p");
        assert(static_cast<dom::TextNode*>(p->first_child())->data() == "Valid");
    }

    // 5. Lone angle brackets in text
    {
        auto doc = html::Parser::parse_string("<p>5 < 10 and 10 > 5</p>");
        assert(doc != nullptr);
        auto* p = doc->document_element();
        assert(p != nullptr && p->tag_name() == "p");
        assert(static_cast<dom::TextNode*>(p->first_child())->data() == "5 < 10 and 10 > 5");
    }

    // 6. Deeply unclosed elements
    {
        auto doc = html::Parser::parse_string("<div><section><article><main>Deep text");
        assert(doc != nullptr);
        assert(doc->document_element() != nullptr);
    }

    std::cout << "[PASS] test_malformed_input\n";
}

void test_tree_traversal() {
    const std::string source =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "  <head>\n"
        "    <title>My First Page</title>\n"
        "  </head>\n"
        "  <body>\n"
        "    <h1>Hello, Divine!</h1>\n"
        "    <p>This is my first rendered document.</p>\n"
        "    <div id=\"content\">\n"
        "      <p>DOM trees are awesome.</p>\n"
        "    </div>\n"
        "  </body>\n"
        "</html>";

    auto doc = html::Parser::parse_string(source);
    assert(doc != nullptr);

    // Document element
    dom::Element* html_elem = doc->document_element();
    assert(html_elem != nullptr && html_elem->tag_name() == "html");

    // Traversal: get_element_by_id
    dom::Element* content_div = doc->get_element_by_id("content");
    assert(content_div != nullptr && content_div->tag_name() == "div");
    assert(content_div->id() == "content");

    // Traversal: get_elements_by_tag_name
    auto p_list = doc->get_elements_by_tag_name("p");
    assert(p_list.size() == 2);
    assert(static_cast<dom::TextNode*>(p_list[0]->first_child())->data() == "This is my first rendered document.");
    assert(static_cast<dom::TextNode*>(p_list[1]->first_child())->data() == "DOM trees are awesome.");

    auto title_list = doc->get_elements_by_tag_name("title");
    assert(title_list.size() == 1);
    assert(static_cast<dom::TextNode*>(title_list[0]->first_child())->data() == "My First Page");

    auto h1_list = doc->get_elements_by_tag_name("h1");
    assert(h1_list.size() == 1);
    assert(static_cast<dom::TextNode*>(h1_list[0]->first_child())->data() == "Hello, Divine!");

    // Dump tree test
    std::string tree_dump = dom::dump_tree(*doc);
    assert(!tree_dump.empty());
    assert(tree_dump.find("#document") != std::string::npos);
    assert(tree_dump.find("<div id=\"content\">") != std::string::npos);

    std::cout << "[PASS] test_tree_traversal\n";
}

int main() {
    std::cout << "Running Divine Engine DOM & HTML Parser Tests...\n";
    test_empty_document();
    test_nested_elements();
    test_preserving_text_content();
    test_multiple_attributes();
    test_comments_and_doctypes();
    test_void_and_self_closing_elements();
    test_malformed_input();
    test_tree_traversal();
    std::cout << "All DOM & HTML Parser Tests Passed Successfully!\n";
    return 0;
}
