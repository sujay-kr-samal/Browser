#include "css/types.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <string>

namespace divine::css {

namespace {

std::string_view trim_whitespace(std::string_view str) {
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front()))) {
        str.remove_prefix(1);
    }
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) {
        str.remove_suffix(1);
    }
    return str;
}

std::string to_lower_ascii(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return result;
}

int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

} // namespace

std::optional<Color> Color::parse(std::string_view str) {
    str = trim_whitespace(str);
    if (str.empty()) {
        return std::nullopt;
    }

    // 1. Named colors
    std::string lower = to_lower_ascii(str);
    if (lower == "transparent") return Color{0, 0, 0, 0};
    if (lower == "black") return Color{0, 0, 0, 255};
    if (lower == "white") return Color{255, 255, 255, 255};
    if (lower == "red") return Color{255, 0, 0, 255};
    if (lower == "green") return Color{0, 128, 0, 255};
    if (lower == "lime") return Color{0, 255, 0, 255};
    if (lower == "blue") return Color{0, 0, 255, 255};
    if (lower == "yellow") return Color{255, 255, 0, 255};
    if (lower == "orange") return Color{255, 165, 0, 255};
    if (lower == "purple") return Color{128, 0, 128, 255};
    if (lower == "gray" || lower == "grey") return Color{128, 128, 128, 255};
    if (lower == "lightgray" || lower == "lightgrey") return Color{211, 211, 211, 255};
    if (lower == "darkgray" || lower == "darkgrey") return Color{169, 169, 169, 255};
    if (lower == "cyan" || lower == "aqua") return Color{0, 255, 255, 255};
    if (lower == "magenta" || lower == "fuchsia") return Color{255, 0, 255, 255};
    if (lower == "silver") return Color{192, 192, 192, 255};
    if (lower == "navy") return Color{0, 0, 128, 255};
    if (lower == "teal") return Color{0, 128, 128, 255};
    if (lower == "maroon") return Color{128, 0, 0, 255};
    if (lower == "olive") return Color{128, 128, 0, 255};

    // 2. Hex colors (#rgb, #rgba, #rrggbb, #rrggbbaa)
    if (str.starts_with('#')) {
        std::string_view hex = str.substr(1);
        if (hex.size() == 3) {
            int r = hex_val(hex[0]);
            int g = hex_val(hex[1]);
            int b = hex_val(hex[2]);
            if (r >= 0 && g >= 0 && b >= 0) {
                return Color{
                    static_cast<uint8_t>(r * 17),
                    static_cast<uint8_t>(g * 17),
                    static_cast<uint8_t>(b * 17),
                    255
                };
            }
        } else if (hex.size() == 4) {
            int r = hex_val(hex[0]);
            int g = hex_val(hex[1]);
            int b = hex_val(hex[2]);
            int a = hex_val(hex[3]);
            if (r >= 0 && g >= 0 && b >= 0 && a >= 0) {
                return Color{
                    static_cast<uint8_t>(r * 17),
                    static_cast<uint8_t>(g * 17),
                    static_cast<uint8_t>(b * 17),
                    static_cast<uint8_t>(a * 17)
                };
            }
        } else if (hex.size() == 6) {
            int r1 = hex_val(hex[0]), r2 = hex_val(hex[1]);
            int g1 = hex_val(hex[2]), g2 = hex_val(hex[3]);
            int b1 = hex_val(hex[4]), b2 = hex_val(hex[5]);
            if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
                return Color{
                    static_cast<uint8_t>((r1 << 4) | r2),
                    static_cast<uint8_t>((g1 << 4) | g2),
                    static_cast<uint8_t>((b1 << 4) | b2),
                    255
                };
            }
        } else if (hex.size() == 8) {
            int r1 = hex_val(hex[0]), r2 = hex_val(hex[1]);
            int g1 = hex_val(hex[2]), g2 = hex_val(hex[3]);
            int b1 = hex_val(hex[4]), b2 = hex_val(hex[5]);
            int a1 = hex_val(hex[6]), a2 = hex_val(hex[7]);
            if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 &&
                b1 >= 0 && b2 >= 0 && a1 >= 0 && a2 >= 0) {
                return Color{
                    static_cast<uint8_t>((r1 << 4) | r2),
                    static_cast<uint8_t>((g1 << 4) | g2),
                    static_cast<uint8_t>((b1 << 4) | b2),
                    static_cast<uint8_t>((a1 << 4) | a2)
                };
            }
        }
    }

    // 3. Functional rgb(r, g, b) and rgba(r, g, b, a)
    if (lower.starts_with("rgb(") && lower.ends_with(')')) {
        std::string_view inner = str.substr(4, str.size() - 5);
        auto first_comma = inner.find(',');
        if (first_comma != std::string_view::npos) {
            auto second_comma = inner.find(',', first_comma + 1);
            if (second_comma != std::string_view::npos) {
                float r = std::strtof(std::string(inner.substr(0, first_comma)).c_str(), nullptr);
                float g = std::strtof(std::string(inner.substr(first_comma + 1, second_comma - first_comma - 1)).c_str(), nullptr);
                float b = std::strtof(std::string(inner.substr(second_comma + 1)).c_str(), nullptr);
                return Color{
                    static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f)),
                    255
                };
            }
        }
    } else if (lower.starts_with("rgba(") && lower.ends_with(')')) {
        std::string_view inner = str.substr(5, str.size() - 6);
        auto c1 = inner.find(',');
        if (c1 != std::string_view::npos) {
            auto c2 = inner.find(',', c1 + 1);
            if (c2 != std::string_view::npos) {
                auto c3 = inner.find(',', c2 + 1);
                if (c3 != std::string_view::npos) {
                    float r = std::strtof(std::string(inner.substr(0, c1)).c_str(), nullptr);
                    float g = std::strtof(std::string(inner.substr(c1 + 1, c2 - c1 - 1)).c_str(), nullptr);
                    float b = std::strtof(std::string(inner.substr(c2 + 1, c3 - c2 - 1)).c_str(), nullptr);
                    float a = std::strtof(std::string(inner.substr(c3 + 1)).c_str(), nullptr);
                    float alpha_255 = (a <= 1.0f) ? (a * 255.0f) : a;
                    return Color{
                        static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f)),
                        static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f)),
                        static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f)),
                        static_cast<uint8_t>(std::clamp(alpha_255, 0.0f, 255.0f))
                    };
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<Length> Length::parse(std::string_view str) {
    str = trim_whitespace(str);
    if (str.empty()) {
        return std::nullopt;
    }

    std::string lower = to_lower_ascii(str);
    if (lower == "auto") {
        return Length::auto_length();
    }
    if (lower == "0" || lower == "0.0") {
        return Length::px(0.0f);
    }

    if (lower.ends_with("px")) {
        std::string num_str(str.substr(0, str.size() - 2));
        char* end = nullptr;
        float val = std::strtof(num_str.c_str(), &end);
        if (end != num_str.c_str()) {
            return Length::px(val);
        }
    }

    return std::nullopt;
}

} // namespace divine::css
