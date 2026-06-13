#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace rhapsode {

/// Replace invalid UTF-8 sequences with U+FFFD so pybind11 string
/// conversion never throws UnicodeDecodeError.
inline std::string sanitize_utf8(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int len = 0;
        if (c < 0x80)        len = 1;
        else if (c < 0xC0)   len = 0;  // invalid continuation byte
        else if (c < 0xE0)   len = 2;
        else if (c < 0xF0)   len = 3;
        else if (c < 0xF8)   len = 4;
        else                  len = 0;  // invalid

        if (len == 0) {
            out += "\xEF\xBF\xBD";  // U+FFFD
            ++i;
            continue;
        }
        if (i + len > s.size()) {
            out += "\xEF\xBF\xBD";
            ++i;
            continue;
        }
        bool valid = true;
        for (int j = 1; j < len; ++j) {
            unsigned char cont = static_cast<unsigned char>(s[i + j]);
            if ((cont & 0xC0) != 0x80) { valid = false; break; }
        }
        if (valid) {
            out.append(s, i, len);
            i += len;
        } else {
            out += "\xEF\xBF\xBD";
            ++i;
        }
    }
    return out;
}

inline bool extract_balanced_json(std::string_view text, std::string& out) {
    auto start = text.find('{');
    if (start == std::string_view::npos) return false;

    int  depth     = 0;
    bool in_string = false;
    bool escaped   = false;

    for (size_t i = start; i < text.size(); ++i) {
        char c = text[i];
        if (in_string) {
            if      (escaped)   escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"')  in_string = false;
        } else {
            if      (c == '"') in_string = true;
            else if (c == '{') ++depth;
            else if (c == '}' && --depth == 0) {
                out.assign(text.data() + start, text.data() + i + 1);
                return true;
            }
        }
    }
    return false;
}

inline nlohmann::json try_parse_json(const std::string& text) {
    try { return nlohmann::json::parse(text); }
    catch (...) {}

    std::string salvaged;
    if (extract_balanced_json(text, salvaged)) {
        try { return nlohmann::json::parse(salvaged); }
        catch (...) {}
    }

    std::cerr << "  [parse] JSON extraction failed -- using empty object\n";
    return nlohmann::json::object();
}

/// Read a numeric field that the LLM may have returned as a quoted string.
template <typename T>
T json_number(const nlohmann::json& j, const char* key, T fallback) {
    auto it = j.find(key);
    if (it == j.end()) return fallback;
    if (it->is_number()) return it->get<T>();
    if (it->is_string()) {
        try { return static_cast<T>(std::stoll(it->get<std::string>())); }
        catch (...) { return fallback; }
    }
    return fallback;
}

}  // namespace rhapsode
