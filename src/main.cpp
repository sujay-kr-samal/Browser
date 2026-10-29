#include "css/parser.hpp"
#include "css/resolver.hpp"
#include "dom/node.hpp"
#include "engine.hpp"
#include "html/parser.hpp"
#include "layout/box.hpp"
#include "layout/tree_builder.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

int main(int argc, char* argv[]) {
    divine::EngineConfig config;
    std::string html_to_parse;
    std::string css_to_parse;
    float layout_viewport_width = 800.0f;
    bool parse_html_mode = false;
    bool parse_css_mode = false;
    bool resolve_mode = false;
    bool layout_mode = false;
    std::string html_file;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--headless") {
            config.headless = true;
        } else if (arg == "--font" && i + 1 < argc) {
            config.font_path = argv[++i];
        } else if (arg == "--open" && i + 1 < argc) {
            html_file = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            config.max_frames = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--parse-html" && i + 1 < argc) {
            html_to_parse = argv[++i];
            parse_html_mode = true;
        } else if (arg == "--parse-file" && i + 1 < argc) {
            const char* path = argv[++i];
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << path << "\n";
                return 1;
            }
            std::ostringstream ss;
            ss << file.rdbuf();
            html_to_parse = ss.str();
            parse_html_mode = true;
        } else if (arg == "--parse-css" && i + 1 < argc) {
            css_to_parse = argv[++i];
            parse_css_mode = true;
        } else if (arg == "--resolve-style" && i + 2 < argc) {
            html_to_parse = argv[++i];
            css_to_parse = argv[++i];
            resolve_mode = true;
        } else if (arg == "--layout" && i + 2 < argc) {
            html_to_parse = argv[++i];
            css_to_parse = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                layout_viewport_width = std::strtof(argv[++i], nullptr);
            }
            layout_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: divine-engine [options]\n"
                      << "Options:\n"
                      << "  --headless                     Run using dummy video driver (no window displayed)\n"
                      << "  --frames <count>               Exit automatically after rendering <count> frames\n"
                      << "  --open <file.html>             Open a local HTML file in the interactive window\n"
                      << "  --font <file.ttf>              Use a specific TrueType/OpenType font file\n"
                      << "  --parse-html \"<html...>\"       Parse HTML string and dump DOM tree\n"
                      << "  --parse-file <file>            Parse HTML file and dump DOM tree\n"
                      << "  --parse-css \"<css...>\"         Parse CSS string and dump rule counts\n"
                      << "  --resolve-style \"<html...>\" \"<css...>\" Resolve computed styles\n"
                      << "  --layout \"<html...>\" \"<css...>\" [w] Compute and dump layout tree\n"
                      << "  --help, -h                     Show this help message\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (parse_html_mode) {
        auto doc = divine::html::Parser::parse_string(html_to_parse);
        std::cout << "Parsed DOM tree:\n" << divine::dom::dump_tree(*doc);
        return 0;
    }

    if (parse_css_mode) {
        auto sheet = divine::css::Parser::parse(css_to_parse);
        std::cout << "Parsed CSS stylesheet: " << sheet.rules.size() << " rule(s).\n";
        for (size_t r = 0; r < sheet.rules.size(); ++r) {
            std::cout << "  Rule " << r << " (" << sheet.rules[r].selectors.size() << " selectors, "
                      << sheet.rules[r].declarations.size() << " declarations):\n";
            for (const auto& d : sheet.rules[r].declarations) {
                std::cout << "    " << d.property << ": " << d.value << ";\n";
            }
        }
        return 0;
    }

    if (resolve_mode) {
        auto doc = divine::html::Parser::parse_string(html_to_parse);
        auto sheet = divine::css::Parser::parse(css_to_parse);
        divine::css::StyleResolver resolver(std::move(sheet));
        auto styles = resolver.resolve_document_styles(*doc);

        std::cout << "Resolved styles for " << styles.size() << " element(s):\n";
        for (const auto& [elem, s] : styles) {
            std::cout << "  <" << elem->tag_name();
            if (!elem->id().empty()) std::cout << " id=\"" << elem->id() << '"';
            if (!elem->class_name().empty()) std::cout << " class=\"" << elem->class_name() << '"';
            std::cout << ">:\n";
            std::cout << "    color: rgb(" << (int)s.color.r << ", " << (int)s.color.g << ", " << (int)s.color.b << ")\n";
            std::cout << "    background-color: rgba(" << (int)s.background_color.r << ", " << (int)s.background_color.g << ", " << (int)s.background_color.b << ", " << (int)s.background_color.a << ")\n";
            std::cout << "    font-size: " << s.font_size << "px, font-weight: " << s.font_weight << "\n";
            std::cout << "    display: " << (s.display == divine::css::Display::Block ? "block" : (s.display == divine::css::Display::Inline ? "inline" : "none")) << "\n";
            std::cout << "    margin: [" << s.margin.top.value << "px, " << s.margin.right.value << "px, " << s.margin.bottom.value << "px, " << s.margin.left.value << "px]\n";
            std::cout << "    padding: [" << s.padding.top.value << "px, " << s.padding.right.value << "px, " << s.padding.bottom.value << "px, " << s.padding.left.value << "px]\n";
        }
        return 0;
    }

    if (layout_mode) {
        auto doc = divine::html::Parser::parse_string(html_to_parse);
        auto sheet = divine::css::Parser::parse(css_to_parse);
        divine::css::StyleResolver resolver(std::move(sheet));
        auto styles = resolver.resolve_document_styles(*doc);

        auto layout_root = divine::layout::LayoutTreeBuilder::build(*doc, styles);
        if (!layout_root) {
            std::cerr << "Failed to build layout tree (document empty or hidden).\n";
            return 1;
        }

        divine::layout::Dimensions viewport_cb;
        viewport_cb.content.width = layout_viewport_width;
        layout_root->layout(viewport_cb);

        std::cout << "Computed Layout Tree (viewport width: " << layout_viewport_width << "px):\n";
        std::cout << divine::layout::dump_layout_tree(*layout_root);
        return 0;
    }

    if (html_file.empty() && argc > 1 && argv[1][0] != '-') {
        html_file = argv[1];
    }

    divine::Engine engine;
    if (!engine.init(config)) {
        std::cerr << "Failed to initialize Divine Engine.\n";
        return 1;
    }

    if (!html_file.empty() && !engine.load_file(html_file)) {
        std::cerr << "[Engine] Page loaded with one or more local resource errors.\n";
    }

    engine.run();

    std::cout << "[Engine] Exited cleanly after " << engine.frame_count() << " frames.\n";
    return 0;
}
