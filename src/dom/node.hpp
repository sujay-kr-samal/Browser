#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace divine::dom {

enum class NodeType {
    Document,
    DocumentType,
    Element,
    Text,
    Comment
};

class Node {
public:
    explicit Node(NodeType type);
    virtual ~Node() = default;

    // Non-copyable, movable
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    [[nodiscard]] NodeType type() const noexcept { return m_type; }
    [[nodiscard]] Node* parent() const noexcept { return m_parent; }
    [[nodiscard]] const std::vector<std::unique_ptr<Node>>& children() const noexcept { return m_children; }

    [[nodiscard]] bool is_document() const noexcept { return m_type == NodeType::Document; }
    [[nodiscard]] bool is_doctype() const noexcept { return m_type == NodeType::DocumentType; }
    [[nodiscard]] bool is_element() const noexcept { return m_type == NodeType::Element; }
    [[nodiscard]] bool is_text() const noexcept { return m_type == NodeType::Text; }
    [[nodiscard]] bool is_comment() const noexcept { return m_type == NodeType::Comment; }

    Node* append_child(std::unique_ptr<Node> child);
    std::unique_ptr<Node> remove_child(Node* child);

    [[nodiscard]] Node* first_child() const noexcept;
    [[nodiscard]] Node* last_child() const noexcept;

    // Preorder depth-first traversal helper
    template <typename Visitor>
    void for_each_descendant(Visitor&& visitor) {
        for (auto& child : m_children) {
            visitor(*child);
            child->for_each_descendant(visitor);
        }
    }

    template <typename Visitor>
    void for_each_descendant(Visitor&& visitor) const {
        for (const auto& child : m_children) {
            visitor(*child);
            child->for_each_descendant(visitor);
        }
    }

private:
    NodeType m_type;
    Node* m_parent{nullptr};
    std::vector<std::unique_ptr<Node>> m_children;
};

struct Attribute {
    std::string name;
    std::string value;

    bool operator==(const Attribute& other) const = default;
};

class Element : public Node {
public:
    explicit Element(std::string tag_name);

    [[nodiscard]] const std::string& tag_name() const noexcept { return m_tag_name; }
    [[nodiscard]] const std::vector<Attribute>& attributes() const noexcept { return m_attributes; }

    [[nodiscard]] bool has_attribute(std::string_view name) const;
    [[nodiscard]] std::optional<std::string_view> get_attribute(std::string_view name) const;
    void set_attribute(std::string name, std::string value);
    bool remove_attribute(std::string_view name);

    [[nodiscard]] std::string_view id() const;
    [[nodiscard]] std::string_view class_name() const;

private:
    std::string m_tag_name;
    std::vector<Attribute> m_attributes;
};

class TextNode : public Node {
public:
    explicit TextNode(std::string text);

    [[nodiscard]] const std::string& data() const noexcept { return m_data; }
    void set_data(std::string text) { m_data = std::move(text); }
    void append_data(std::string_view text) { m_data.append(text); }

private:
    std::string m_data;
};

class CommentNode : public Node {
public:
    explicit CommentNode(std::string comment);

    [[nodiscard]] const std::string& data() const noexcept { return m_data; }

private:
    std::string m_data;
};

class DocumentTypeNode : public Node {
public:
    explicit DocumentTypeNode(std::string name);

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

private:
    std::string m_name;
};

class Document : public Node {
public:
    Document();

    [[nodiscard]] Element* document_element() const;
    [[nodiscard]] Element* get_element_by_id(std::string_view id) const;
    [[nodiscard]] std::vector<Element*> get_elements_by_tag_name(std::string_view tag) const;

    [[nodiscard]] const std::string& doctype() const noexcept { return m_doctype; }
    void set_doctype(std::string doctype) { m_doctype = std::move(doctype); }

private:
    std::string m_doctype;
};

// Formats a DOM subtree into an indented string representation for debugging and testing
std::string dump_tree(const Node& root, int depth = 0);

} // namespace divine::dom
