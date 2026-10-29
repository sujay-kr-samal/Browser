#include "dom/node.hpp"

#include <algorithm>
#include <sstream>

namespace divine::dom {

Node::Node(NodeType type) : m_type(type) {}

Node* Node::append_child(std::unique_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    child->m_parent = this;
    m_children.push_back(std::move(child));
    return m_children.back().get();
}

std::unique_ptr<Node> Node::remove_child(Node* child) {
    if (!child) {
        return nullptr;
    }
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<Node>& ptr) { return ptr.get() == child; });
    if (it != m_children.end()) {
        std::unique_ptr<Node> removed = std::move(*it);
        m_children.erase(it);
        removed->m_parent = nullptr;
        return removed;
    }
    return nullptr;
}

Node* Node::first_child() const noexcept {
    return m_children.empty() ? nullptr : m_children.front().get();
}

Node* Node::last_child() const noexcept {
    return m_children.empty() ? nullptr : m_children.back().get();
}

Element::Element(std::string tag_name)
    : Node(NodeType::Element), m_tag_name(std::move(tag_name)) {}

bool Element::has_attribute(std::string_view name) const {
    return std::any_of(m_attributes.begin(), m_attributes.end(),
        [name](const Attribute& attr) { return attr.name == name; });
}

std::optional<std::string_view> Element::get_attribute(std::string_view name) const {
    auto it = std::find_if(m_attributes.begin(), m_attributes.end(),
        [name](const Attribute& attr) { return attr.name == name; });
    if (it != m_attributes.end()) {
        return it->value;
    }
    return std::nullopt;
}

void Element::set_attribute(std::string name, std::string value) {
    auto it = std::find_if(m_attributes.begin(), m_attributes.end(),
        [&name](const Attribute& attr) { return attr.name == name; });
    if (it != m_attributes.end()) {
        it->value = std::move(value);
    } else {
        m_attributes.push_back(Attribute{std::move(name), std::move(value)});
    }
}

bool Element::remove_attribute(std::string_view name) {
    auto it = std::find_if(m_attributes.begin(), m_attributes.end(),
        [name](const Attribute& attr) { return attr.name == name; });
    if (it != m_attributes.end()) {
        m_attributes.erase(it);
        return true;
    }
    return false;
}

std::string_view Element::id() const {
    auto attr = get_attribute("id");
    return attr.value_or(std::string_view{});
}

std::string_view Element::class_name() const {
    auto attr = get_attribute("class");
    return attr.value_or(std::string_view{});
}

TextNode::TextNode(std::string text)
    : Node(NodeType::Text), m_data(std::move(text)) {}

CommentNode::CommentNode(std::string comment)
    : Node(NodeType::Comment), m_data(std::move(comment)) {}

DocumentTypeNode::DocumentTypeNode(std::string name)
    : Node(NodeType::DocumentType), m_name(std::move(name)) {}

Document::Document() : Node(NodeType::Document) {}

Element* Document::document_element() const {
    for (const auto& child : children()) {
        if (child->is_element()) {
            return static_cast<Element*>(child.get());
        }
    }
    return nullptr;
}

Element* Document::get_element_by_id(std::string_view id) const {
    Element* found = nullptr;
    for_each_descendant([&](const Node& node) {
        if (!found && node.is_element()) {
            const auto& elem = static_cast<const Element&>(node);
            if (elem.id() == id) {
                found = const_cast<Element*>(&elem);
            }
        }
    });
    return found;
}

std::vector<Element*> Document::get_elements_by_tag_name(std::string_view tag) const {
    std::vector<Element*> results;
    for_each_descendant([&](const Node& node) {
        if (node.is_element()) {
            const auto& elem = static_cast<const Element&>(node);
            if (elem.tag_name() == tag) {
                results.push_back(const_cast<Element*>(&elem));
            }
        }
    });
    return results;
}

std::string dump_tree(const Node& root, int depth) {
    std::ostringstream ss;
    const std::string indent(depth * 2, ' ');

    switch (root.type()) {
        case NodeType::Document: {
            const auto& doc = static_cast<const Document&>(root);
            ss << indent << "#document";
            if (!doc.doctype().empty()) {
                ss << " [doctype: " << doc.doctype() << "]";
            }
            ss << '\n';
            break;
        }
        case NodeType::DocumentType: {
            const auto& dt = static_cast<const DocumentTypeNode&>(root);
            ss << indent << "<!DOCTYPE " << dt.name() << ">\n";
            break;
        }
        case NodeType::Element: {
            const auto& elem = static_cast<const Element&>(root);
            ss << indent << '<' << elem.tag_name();
            for (const auto& attr : elem.attributes()) {
                ss << ' ' << attr.name << "=\"" << attr.value << '"';
            }
            ss << ">\n";
            break;
        }
        case NodeType::Text: {
            const auto& txt = static_cast<const TextNode&>(root);
            ss << indent << "#text \"" << txt.data() << "\"\n";
            break;
        }
        case NodeType::Comment: {
            const auto& cmt = static_cast<const CommentNode&>(root);
            ss << indent << "<!-- " << cmt.data() << " -->\n";
            break;
        }
    }

    for (const auto& child : root.children()) {
        ss << dump_tree(*child, depth + 1);
    }

    return ss.str();
}

} // namespace divine::dom
