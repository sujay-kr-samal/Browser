#pragma once

#include "css/stylesheet.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace divine::css {

class Parser {
public:
    explicit Parser(std::string_view source);

    Stylesheet parse_stylesheet();

    static Stylesheet parse(std::string_view source);
    static std::vector<Declaration> parse_inline_style(std::string_view inline_style);
    static std::optional<Selector> parse_single_selector(std::string_view selector_str);

private:
    [[nodiscard]] bool is_eof() const noexcept { return m_pos >= m_source.size(); }
    [[nodiscard]] char peek(size_t offset = 0) const noexcept;
    char advance() noexcept;

    void skip_whitespace_and_comments() noexcept;

    std::vector<Selector> parse_selector_list();
    std::optional<Selector> parse_selector();
    std::optional<SimpleSelector> parse_simple_selector();

    std::vector<Declaration> parse_declaration_block();
    std::optional<Declaration> parse_declaration();

    std::string_view m_source;
    size_t m_pos{0};
};

} // namespace divine::css
