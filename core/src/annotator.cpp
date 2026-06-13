#include "rhapsode/annotator.h"

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

namespace rhapsode {

namespace {

std::string to_lower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
        out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

bool is_word_boundary(const std::string& text, size_t pos) {
    return pos >= text.size() || !std::isalnum(static_cast<unsigned char>(text[pos]));
}

}  // namespace

Annotator::Annotator(const Scene& scene) : scene_(scene) {}

void Annotator::set_ner_callback(NERCallback cb) { ner_cb_ = std::move(cb); }

std::vector<EntitySpan> Annotator::match_characters(const std::string& text) const {
    std::vector<EntitySpan> spans;
    std::string text_lower = to_lower(text);

    for (const auto& ch : scene_.characters) {
        if (ch.name.empty()) continue;
        std::string name_lower = to_lower(ch.name);
        size_t pos = 0;
        while ((pos = text_lower.find(name_lower, pos)) != std::string::npos) {
            size_t end = pos + ch.name.size();
            bool left_ok  = (pos == 0) || is_word_boundary(text, pos - 1);
            bool right_ok = is_word_boundary(text, end);
            if (left_ok && right_ok)
                spans.push_back({static_cast<int>(pos), static_cast<int>(end),
                                 text.substr(pos, ch.name.size()), "character"});
            pos = end;
        }
    }
    return spans;
}

std::vector<EntitySpan> Annotator::annotate(const std::string& text) const {
    auto gamestate = match_characters(text);

    std::vector<EntitySpan> ner_spans;
    if (ner_cb_) {
        auto json_str = ner_cb_(text);
        auto arr = nlohmann::json::parse(json_str, nullptr, false);
        if (arr.is_array()) {
            for (const auto& e : arr) {
                ner_spans.push_back({
                    e.value("start", 0), e.value("end", 0),
                    e.value("text", ""), e.value("category", "entity")
                });
            }
        }
    }

    return merge(std::move(gamestate), std::move(ner_spans));
}

std::vector<EntitySpan> Annotator::merge(std::vector<EntitySpan> gs,
                                          std::vector<EntitySpan> ner) {
    for (auto it = ner.begin(); it != ner.end(); ) {
        bool overlaps = false;
        for (const auto& g : gs) {
            if (it->start < g.end && g.start < it->end) { overlaps = true; break; }
        }
        it = overlaps ? ner.erase(it) : std::next(it);
    }

    gs.insert(gs.end(), std::make_move_iterator(ner.begin()),
                        std::make_move_iterator(ner.end()));
    std::sort(gs.begin(), gs.end(), [](const EntitySpan& a, const EntitySpan& b) {
        return a.start < b.start;
    });
    return gs;
}

}  // namespace rhapsode
