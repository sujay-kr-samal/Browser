#pragma once

#include "dom/node.hpp"
#include "html/tokenizer.hpp"
#include <memory>
#include <string_view>
#include <vector>

namespace divine::html {

// Standard HTML void elements that never have child nodes or end tags
bool is_void_element(std::string_view tag_name) noexcept;

class Parser {
public:
    explicit Parser(std::string_view html);

    std::unique_ptr<dom::Document> parse();

    static std::unique_ptr<dom::Document> parse_string(std::string_view html);

private:
    dom::Node* current_node();
    void handle_token(Token& token);

    Tokenizer m_tokenizer;
    std::unique_ptr<dom::Document> m_document;
    std::vector<dom::Element*> m_open_elements;
};

} // namespace divine::html
