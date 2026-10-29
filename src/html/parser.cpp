#include "html/parser.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace divine::html {

bool is_void_element(std::string_view tag_name) noexcept {
    static constexpr std::array<std::string_view, 14> void_tags = {
        "area", "base", "br", "col", "embed", "hr", "img",
        "input", "link", "meta", "param", "source", "track", "wbr"
    };

    return std::any_of(void_tags.begin(), void_tags.end(),
        [tag_name](std::string_view void_tag) { return tag_name == void_tag; });
}

Parser::Parser(std::string_view html)
    : m_tokenizer(html), m_document(std::make_unique<dom::Document>()) {}

dom::Node* Parser::current_node() {
    if (m_open_elements.empty()) {
        return m_document.get();
    }
    return m_open_elements.back();
}

void Parser::handle_token(Token& token) {
    switch (token.type) {
        case TokenType::DocType: {
            m_document->set_doctype(token.tag_name);
            m_document->append_child(std::make_unique<dom::DocumentTypeNode>(token.tag_name));
            break;
        }

        case TokenType::Comment: {
            current_node()->append_child(std::make_unique<dom::CommentNode>(std::move(token.data)));
            break;
        }

        case TokenType::Character: {
            if (token.data.empty()) {
                break;
            }
            dom::Node* curr = current_node();
            dom::Node* last = curr->last_child();
            if (last != nullptr && last->is_text()) {
                static_cast<dom::TextNode*>(last)->append_data(token.data);
            } else {
                curr->append_child(std::make_unique<dom::TextNode>(std::move(token.data)));
            }
            break;
        }

        case TokenType::StartTag: {
            auto elem = std::make_unique<dom::Element>(token.tag_name);
            for (auto& attr : token.attributes) {
                elem->set_attribute(std::move(attr.name), std::move(attr.value));
            }

            const bool is_void = is_void_element(elem->tag_name()) || token.self_closing;
            dom::Element* elem_ptr = static_cast<dom::Element*>(
                current_node()->append_child(std::move(elem))
            );

            if (!is_void) {
                m_open_elements.push_back(elem_ptr);
            }
            break;
        }

        case TokenType::EndTag: {
            for (auto it = m_open_elements.rbegin(); it != m_open_elements.rend(); ++it) {
                if ((*it)->tag_name() == token.tag_name) {
                    auto forward_it = it.base(); // forward_it - 1 points to the matched element
                    m_open_elements.erase(forward_it - 1, m_open_elements.end());
                    break;
                }
            }
            break;
        }

        case TokenType::EndOfFile: {
            m_open_elements.clear();
            break;
        }
    }
}

std::unique_ptr<dom::Document> Parser::parse() {
    while (true) {
        Token token = m_tokenizer.next_token();
        const bool is_eof = (token.type == TokenType::EndOfFile);
        handle_token(token);
        if (is_eof) {
            break;
        }
    }
    return std::move(m_document);
}

std::unique_ptr<dom::Document> Parser::parse_string(std::string_view html) {
    Parser parser(html);
    return parser.parse();
}

} // namespace divine::html
