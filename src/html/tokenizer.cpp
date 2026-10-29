#include "html/tokenizer.hpp"

#include <cctype>
#include <sstream>

namespace divine::html {

namespace {

std::string to_lower_ascii(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

} // namespace

std::string decode_html_entities(std::string_view text) {
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            std::string_view remainder = text.substr(i);
            if (remainder.starts_with("&amp;")) {
                result.push_back('&');
                i += 4;
            } else if (remainder.starts_with("&lt;")) {
                result.push_back('<');
                i += 3;
            } else if (remainder.starts_with("&gt;")) {
                result.push_back('>');
                i += 3;
            } else if (remainder.starts_with("&quot;")) {
                result.push_back('"');
                i += 5;
            } else if (remainder.starts_with("&apos;")) {
                result.push_back('\'');
                i += 5;
            } else {
                result.push_back('&');
            }
        } else {
            result.push_back(text[i]);
        }
    }
    return result;
}

Tokenizer::Tokenizer(std::string_view source) : m_source(source), m_pos(0) {}

char Tokenizer::peek(size_t offset) const noexcept {
    if (m_pos + offset < m_source.size()) {
        return m_source[m_pos + offset];
    }
    return '\0';
}

char Tokenizer::advance() noexcept {
    if (m_pos < m_source.size()) {
        return m_source[m_pos++];
    }
    return '\0';
}

bool Tokenizer::match_ci(std::string_view expected) {
    if (m_pos + expected.size() > m_source.size()) {
        return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(m_source[m_pos + i])) !=
            std::tolower(static_cast<unsigned char>(expected[i]))) {
            return false;
        }
    }
    m_pos += expected.size();
    return true;
}

void Tokenizer::skip_whitespace() noexcept {
    while (!is_eof() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

Token Tokenizer::consume_comment() {
    // Already matched "<!--"
    const size_t start = m_pos;
    while (!is_eof()) {
        if (m_source.substr(m_pos).starts_with("-->")) {
            std::string text(m_source.substr(start, m_pos - start));
            m_pos += 3; // skip "-->"
            return Token::make_comment(std::move(text));
        }
        advance();
    }
    // Malformed/unclosed comment at EOF
    return Token::make_comment(std::string(m_source.substr(start)));
}

Token Tokenizer::consume_doctype() {
    // Already matched "<!doctype" (case-insensitive)
    skip_whitespace();
    const size_t name_start = m_pos;
    while (!is_eof() && !std::isspace(static_cast<unsigned char>(peek())) && peek() != '>') {
        advance();
    }
    std::string name = to_lower_ascii(m_source.substr(name_start, m_pos - name_start));

    // Consume remaining characters until closing '>' or EOF
    while (!is_eof() && peek() != '>') {
        advance();
    }
    if (peek() == '>') {
        advance();
    }

    return Token::make_doctype(std::move(name));
}

Token Tokenizer::consume_tag() {
    // Already consumed '<'
    if (peek() == '/') {
        // End tag: </tag>
        advance(); // consume '/'
        skip_whitespace();

        const size_t start = m_pos;
        while (!is_eof() && !std::isspace(static_cast<unsigned char>(peek())) && peek() != '>') {
            advance();
        }
        std::string tag_name = to_lower_ascii(m_source.substr(start, m_pos - start));

        // Skip any trailing garbage/attributes in end tag until '>' or EOF
        while (!is_eof() && peek() != '>') {
            advance();
        }
        if (peek() == '>') {
            advance();
        }

        return Token::make_end_tag(std::move(tag_name));
    }

    // Check if valid start tag name
    if (!std::isalpha(static_cast<unsigned char>(peek()))) {
        // Stray '<' not followed by tag name -> treat as character token
        return Token::make_character("<");
    }

    // Start tag: read tag name
    const size_t start = m_pos;
    while (!is_eof() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                         peek() == '-' || peek() == '_' || peek() == ':')) {
        advance();
    }
    std::string tag_name = to_lower_ascii(m_source.substr(start, m_pos - start));

    std::vector<AttributeToken> attributes;
    bool self_closing = false;

    // Parse attributes
    while (!is_eof()) {
        skip_whitespace();

        if (is_eof()) {
            break;
        }

        if (peek() == '>') {
            advance();
            break;
        }

        if (peek() == '/') {
            advance();
            skip_whitespace();
            if (peek() == '>') {
                advance();
                self_closing = true;
                break;
            }
            continue;
        }

        // Attribute name
        const size_t attr_name_start = m_pos;
        while (!is_eof() && !std::isspace(static_cast<unsigned char>(peek())) &&
               peek() != '=' && peek() != '/' && peek() != '>') {
            advance();
        }

        if (attr_name_start == m_pos) {
            // Unrecognized character: advance to prevent infinite loop
            advance();
            continue;
        }

        std::string attr_name = to_lower_ascii(m_source.substr(attr_name_start, m_pos - attr_name_start));
        std::string attr_value;

        skip_whitespace();

        if (!is_eof() && peek() == '=') {
            advance(); // consume '='
            skip_whitespace();

            if (!is_eof()) {
                if (peek() == '"' || peek() == '\'') {
                    const char quote = advance();
                    const size_t val_start = m_pos;
                    while (!is_eof() && peek() != quote) {
                        advance();
                    }
                    attr_value = decode_html_entities(m_source.substr(val_start, m_pos - val_start));
                    if (!is_eof() && peek() == quote) {
                        advance(); // consume closing quote
                    }
                } else {
                    // Unquoted attribute value
                    const size_t val_start = m_pos;
                    while (!is_eof() && !std::isspace(static_cast<unsigned char>(peek())) &&
                           peek() != '>') {
                        advance();
                    }
                    attr_value = decode_html_entities(m_source.substr(val_start, m_pos - val_start));
                }
            }
        }

        attributes.push_back(AttributeToken{
            .name = std::move(attr_name),
            .value = std::move(attr_value)
        });
    }

    return Token::make_start_tag(std::move(tag_name), std::move(attributes), self_closing);
}

Token Tokenizer::consume_text() {
    const size_t start = m_pos;
    while (!is_eof() && peek() != '<') {
        advance();
    }
    std::string text = decode_html_entities(m_source.substr(start, m_pos - start));
    return Token::make_character(std::move(text));
}

Token Tokenizer::next_token() {
    if (is_eof()) {
        return Token::make_eof();
    }

    if (peek() == '<') {
        const std::string_view remaining = m_source.substr(m_pos);
        if (remaining.starts_with("<!--")) {
            m_pos += 4;
            return consume_comment();
        }
        if (match_ci("<!doctype")) {
            return consume_doctype();
        }
        advance(); // consume '<'
        return consume_tag();
    }

    return consume_text();
}

std::vector<Token> Tokenizer::tokenize_all() {
    std::vector<Token> tokens;
    while (true) {
        Token token = next_token();
        const bool is_eof_token = (token.type == TokenType::EndOfFile);
        tokens.push_back(std::move(token));
        if (is_eof_token) {
            break;
        }
    }
    return tokens;
}

} // namespace divine::html
