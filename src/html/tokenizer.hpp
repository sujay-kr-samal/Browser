#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace divine::html {

enum class TokenType {
    DocType,
    StartTag,
    EndTag,
    Comment,
    Character,
    EndOfFile
};

struct AttributeToken {
    std::string name{};
    std::string value{};

    bool operator==(const AttributeToken& other) const = default;
};

struct Token {
    TokenType type{TokenType::EndOfFile};
    std::string tag_name{};
    std::string data{};
    std::vector<AttributeToken> attributes{};
    bool self_closing{false};

    static Token make_eof() {
        return Token{.type = TokenType::EndOfFile, .tag_name = {}, .data = {}, .attributes = {}, .self_closing = false};
    }

    static Token make_character(std::string data) {
        return Token{.type = TokenType::Character, .tag_name = {}, .data = std::move(data), .attributes = {}, .self_closing = false};
    }

    static Token make_comment(std::string data) {
        return Token{.type = TokenType::Comment, .tag_name = {}, .data = std::move(data), .attributes = {}, .self_closing = false};
    }

    static Token make_doctype(std::string name) {
        return Token{.type = TokenType::DocType, .tag_name = std::move(name), .data = {}, .attributes = {}, .self_closing = false};
    }

    static Token make_start_tag(std::string tag_name, std::vector<AttributeToken> attributes, bool self_closing = false) {
        return Token{.type = TokenType::StartTag, .tag_name = std::move(tag_name), .data = {}, .attributes = std::move(attributes), .self_closing = self_closing};
    }

    static Token make_end_tag(std::string tag_name) {
        return Token{.type = TokenType::EndTag, .tag_name = std::move(tag_name), .data = {}, .attributes = {}, .self_closing = false};
    }

    bool operator==(const Token& other) const = default;
};

class Tokenizer {
public:
    explicit Tokenizer(std::string_view source);

    Token next_token();
    std::vector<Token> tokenize_all();

private:
    [[nodiscard]] bool is_eof() const noexcept { return m_pos >= m_source.size(); }
    [[nodiscard]] char peek(size_t offset = 0) const noexcept;
    char advance() noexcept;
    bool match_ci(std::string_view expected);
    void skip_whitespace() noexcept;

    Token consume_comment();
    Token consume_doctype();
    Token consume_tag();
    Token consume_text();

    std::string_view m_source;
    size_t m_pos{0};
};

std::string decode_html_entities(std::string_view text);

} // namespace divine::html
