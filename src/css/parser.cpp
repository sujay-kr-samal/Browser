#include "css/parser.hpp"

#include <cctype>

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

std::string_view trim_whitespace(std::string_view str) {
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front()))) {
        str.remove_prefix(1);
    }
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) {
        str.remove_suffix(1);
    }
    return str;
}

} // namespace

Parser::Parser(std::string_view source) : m_source(source), m_pos(0) {}

char Parser::peek(size_t offset) const noexcept {
    if (m_pos + offset < m_source.size()) {
        return m_source[m_pos + offset];
    }
    return '\0';
}

char Parser::advance() noexcept {
    if (m_pos < m_source.size()) {
        return m_source[m_pos++];
    }
    return '\0';
}

void Parser::skip_whitespace_and_comments() noexcept {
    while (!is_eof()) {
        if (std::isspace(static_cast<unsigned char>(peek()))) {
            advance();
        } else if (peek() == '/' && peek(1) == '*') {
            advance(); // consume '/'
            advance(); // consume '*'
            while (!is_eof() && !(peek() == '*' && peek(1) == '/')) {
                advance();
            }
            if (!is_eof()) {
                advance(); // consume '*'
                advance(); // consume '/'
            }
        } else {
            break;
        }
    }
}

std::optional<SimpleSelector> Parser::parse_simple_selector() {
    SimpleSelector simple{};

    // 1. Tag name or universal '*'
    if (std::isalpha(static_cast<unsigned char>(peek())) || peek() == '_' || peek() == '-' || peek() == '*') {
        const size_t start = m_pos;
        if (peek() == '*') {
            advance();
        } else {
            while (!is_eof() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                                 peek() == '_' || peek() == '-')) {
                advance();
            }
        }
        simple.type_name = to_lower_ascii(m_source.substr(start, m_pos - start));
    }

    // 2. Class names and IDs attached to this selector
    while (!is_eof()) {
        if (peek() == '.') {
            advance(); // consume '.'
            const size_t start = m_pos;
            while (!is_eof() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                                 peek() == '_' || peek() == '-')) {
                advance();
            }
            if (start < m_pos) {
                simple.classes.push_back(to_lower_ascii(m_source.substr(start, m_pos - start)));
            }
        } else if (peek() == '#') {
            advance(); // consume '#'
            const size_t start = m_pos;
            while (!is_eof() && (std::isalnum(static_cast<unsigned char>(peek())) ||
                                 peek() == '_' || peek() == '-')) {
                advance();
            }
            if (start < m_pos) {
                simple.id = std::string(m_source.substr(start, m_pos - start));
            }
        } else {
            break;
        }
    }

    if (simple.empty()) {
        return std::nullopt;
    }

    return simple;
}

std::optional<Selector> Parser::parse_selector() {
    std::vector<SimpleSelector> components;

    while (!is_eof()) {
        auto simple = parse_simple_selector();
        if (!simple) {
            break;
        }
        components.push_back(std::move(*simple));

        // In CSS, whitespace separating simple selectors indicates a descendant combinator
        bool had_whitespace = false;
        while (!is_eof() && (std::isspace(static_cast<unsigned char>(peek())) ||
                             (peek() == '/' && peek(1) == '*'))) {
            if (peek() == '/' && peek(1) == '*') {
                advance(); advance();
                while (!is_eof() && !(peek() == '*' && peek(1) == '/')) {
                    advance();
                }
                if (!is_eof()) { advance(); advance(); }
            } else {
                advance();
                had_whitespace = true;
            }
        }

        if (is_eof() || peek() == ',' || peek() == '{') {
            break;
        }

        if (!had_whitespace) {
            break;
        }
    }

    if (components.empty()) {
        return std::nullopt;
    }

    return Selector{.components = std::move(components)};
}

std::vector<Selector> Parser::parse_selector_list() {
    std::vector<Selector> selectors;

    while (!is_eof()) {
        skip_whitespace_and_comments();
        auto sel = parse_selector();
        if (sel) {
            selectors.push_back(std::move(*sel));
        }

        skip_whitespace_and_comments();
        if (peek() == ',') {
            advance(); // consume ','
        } else {
            break;
        }
    }

    return selectors;
}

std::optional<Declaration> Parser::parse_declaration() {
    skip_whitespace_and_comments();
    if (is_eof() || peek() == '}') {
        return std::nullopt;
    }

    // Property name
    const size_t prop_start = m_pos;
    while (!is_eof() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '-')) {
        advance();
    }

    if (prop_start == m_pos) {
        // Unexpected character, advance to recover
        advance();
        return std::nullopt;
    }

    std::string prop = to_lower_ascii(m_source.substr(prop_start, m_pos - prop_start));

    skip_whitespace_and_comments();
    if (peek() != ':') {
        // Malformed declaration: skip until ';' or '}'
        while (!is_eof() && peek() != ';' && peek() != '}') {
            advance();
        }
        return std::nullopt;
    }
    advance(); // consume ':'

    skip_whitespace_and_comments();

    // Property value: consume until ';' or '}', respecting parentheses (for rgb, etc.)
    const size_t val_start = m_pos;
    int paren_depth = 0;
    while (!is_eof()) {
        const char c = peek();
        if (c == '(') {
            ++paren_depth;
        } else if (c == ')') {
            if (paren_depth > 0) {
                --paren_depth;
            }
        } else if (paren_depth == 0 && (c == ';' || c == '}')) {
            break;
        }
        advance();
    }

    std::string_view val = trim_whitespace(m_source.substr(val_start, m_pos - val_start));

    return Declaration{
        .property = std::move(prop),
        .value = std::string(val)
    };
}

std::vector<Declaration> Parser::parse_declaration_block() {
    std::vector<Declaration> declarations;

    if (peek() == '{') {
        advance(); // consume '{'
    }

    while (!is_eof()) {
        skip_whitespace_and_comments();
        if (is_eof() || peek() == '}') {
            break;
        }

        auto decl = parse_declaration();
        if (decl) {
            declarations.push_back(std::move(*decl));
        }

        skip_whitespace_and_comments();
        if (peek() == ';') {
            advance();
        }
    }

    if (peek() == '}') {
        advance(); // consume '}'
    }

    return declarations;
}

Stylesheet Parser::parse_stylesheet() {
    Stylesheet sheet{};
    size_t order = 0;

    while (!is_eof()) {
        skip_whitespace_and_comments();
        if (is_eof()) {
            break;
        }

        auto selectors = parse_selector_list();
        skip_whitespace_and_comments();

        if (peek() != '{') {
            // Malformed rule or unhandled at-rule: skip to next '{' block or ';'
            while (!is_eof() && peek() != '{' && peek() != ';') {
                advance();
            }
            if (peek() == '{') {
                int depth = 1;
                advance();
                while (!is_eof() && depth > 0) {
                    if (peek() == '{') ++depth;
                    else if (peek() == '}') --depth;
                    advance();
                }
            } else if (peek() == ';') {
                advance();
            }
            continue;
        }

        auto declarations = parse_declaration_block();

        if (!selectors.empty()) {
            sheet.rules.push_back(StyleRule{
                .selectors = std::move(selectors),
                .declarations = std::move(declarations),
                .source_order = order++
            });
        }
    }

    return sheet;
}

Stylesheet Parser::parse(std::string_view source) {
    Parser parser(source);
    return parser.parse_stylesheet();
}

std::vector<Declaration> Parser::parse_inline_style(std::string_view inline_style) {
    Parser parser(inline_style);
    std::vector<Declaration> decls;

    while (!parser.is_eof()) {
        parser.skip_whitespace_and_comments();
        if (parser.is_eof()) {
            break;
        }
        auto d = parser.parse_declaration();
        if (d) {
            decls.push_back(std::move(*d));
        }
        parser.skip_whitespace_and_comments();
        if (parser.peek() == ';') {
            parser.advance();
        }
    }

    return decls;
}

std::optional<Selector> Parser::parse_single_selector(std::string_view selector_str) {
    Parser parser(selector_str);
    parser.skip_whitespace_and_comments();
    return parser.parse_selector();
}

} // namespace divine::css
