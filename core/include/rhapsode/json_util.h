#pragma once

#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "rhapsode/log_util.h"

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

/// Truncate to at most max_len bytes without splitting a UTF-8 code point.
inline std::string truncate_utf8(const std::string& s, size_t max_len) {
    if (s.size() <= max_len) return s;
    size_t pos = max_len;
    while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
        --pos;
    return s.substr(0, pos);
}

/// Normalize "smart"/typographic punctuation that LLMs habitually emit into the
/// ASCII forms JSON requires.  Curly double quotes (U+201C/D/E) become ", curly
/// single quotes / primes (U+2018/19, U+2032) become ' (valid inside JSON
/// strings).  Everything else is left untouched.  Without this, a model that
/// "prettifies" its JSON with smart quotes produces unparseable output and the
/// whole turn's structured plan is lost.
inline std::string normalize_json_punct(std::string s) {
    struct Repl { const char* from; char to; };
    static const Repl repls[] = {
        {"\xE2\x80\x9C", '"'},   // U+201C left double quote
        {"\xE2\x80\x9D", '"'},   // U+201D right double quote
        {"\xE2\x80\x9E", '"'},   // U+201E low double quote
        {"\xE2\x80\x98", '\''},  // U+2018 left single quote
        {"\xE2\x80\x99", '\''},  // U+2019 right single quote
        {"\xE2\x80\xB2", '\''},  // U+2032 prime
    };
    for (const auto& r : repls) {
        const std::string from = r.from;
        std::string::size_type pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), 1, r.to);
            pos += 1;
        }
    }
    return s;
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

    // Retry after normalizing smart quotes -- the most common reason a model's
    // JSON fails to parse.
    const std::string norm = normalize_json_punct(text);

    std::string salvaged;
    if (extract_balanced_json(norm, salvaged)) {
        try { return nlohmann::json::parse(salvaged); }
        catch (...) {}
    }
    try { return nlohmann::json::parse(norm); }
    catch (...) {}

    log() << "  [parse] JSON extraction failed -- using empty object\n";
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
