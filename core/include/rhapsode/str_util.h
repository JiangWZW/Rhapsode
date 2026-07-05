#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace rhapsode::str {

inline std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

inline std::string to_lower(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

inline bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    return std::equal(a.begin(), a.end(), b.begin(), [](char ca, char cb) {
        return std::tolower(static_cast<unsigned char>(ca)) ==
               std::tolower(static_cast<unsigned char>(cb));
    });
}

inline bool is_word_boundary(std::string_view text, size_t pos) {
    return pos >= text.size() || !std::isalnum(static_cast<unsigned char>(text[pos]));
}

inline bool has_word_match(std::string_view text_lower, std::string_view needle_lower) {
    if (needle_lower.empty()) {
        return false;
    }

    size_t pos = 0;
    while ((pos = text_lower.find(needle_lower, pos)) != std::string_view::npos) {
        const bool left_ok = pos == 0 || is_word_boundary(text_lower, pos - 1);
        const bool right_ok = is_word_boundary(text_lower, pos + needle_lower.size());
        if (left_ok && right_ok) {
            return true;
        }
        pos += needle_lower.size();
    }
    return false;
}

}  // namespace rhapsode::str
