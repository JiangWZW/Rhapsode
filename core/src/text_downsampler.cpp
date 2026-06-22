#include "rhapsode/text_downsampler.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

namespace rhapsode {

namespace {

constexpr int VERBATIM_TAIL_DEFAULT = 6;
constexpr int BATCH_SIZE = 3;
constexpr int MIP0_MAX = 10;
constexpr int MIP1_MAX = 5;
constexpr int MIP2_MAX = 3;
constexpr int SNIPPETS_PER_PROMOTION = 2;
constexpr int MAX_MIPS = 3;

constexpr const char* SUMMARIZER_SYSTEM =
    "You are a precise narrative-state tracker. "
    "Output only the summary line.";

constexpr const char* SUMMARIZER_USER_FMT =
    "<prior_context>%s</prior_context>\n"
    "<passage>%s</passage>\n"
    "Summarize only new elements not in prior_context. Short phrases, single line.";

double now_seconds() {
    auto tp = std::chrono::system_clock::now();
    return std::chrono::duration<double>(tp.time_since_epoch()).count();
}

std::string role_to_string(Role r) {
    switch (r) {
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        case Role::System:    return "system";
    }
    return "unknown";
}

}  // namespace

TextDownsampler::TextDownsampler() {
    levels_.resize(MAX_MIPS);
    levels_[0].max_snippets = MIP0_MAX;
    levels_[1].max_snippets = MIP1_MAX;
    levels_[2].max_snippets = MIP2_MAX;
}

void TextDownsampler::set_llm_callback(DownsamplerLLMCallback cb) {
    llm_cb_ = std::move(cb);
}

void TextDownsampler::process_turn(const std::vector<SceneMessage>& messages,
                                    int verbatim_tail) {
    int total = static_cast<int>(messages.size());
    int eligible_end = std::max(0, total - verbatim_tail);

    while (summarized_up_to_ + BATCH_SIZE <= eligible_end) {
        int batch_start = summarized_up_to_;
        int batch_end = batch_start + BATCH_SIZE;

        std::string batch_text;
        for (int i = batch_start; i < batch_end; ++i) {
            if (!batch_text.empty()) batch_text += '\n';
            batch_text += role_to_string(messages[i].role) + ": " + messages[i].content;
        }

        auto summary = summarize(batch_text, 0);
        if (summary.empty())
            break;

        Snippet s;
        s.text = std::move(summary);
        s.turn_start = batch_start;
        s.turn_end = batch_end - 1;
        s.timestamp = now_seconds();
        s.merged_count = 1;
        levels_[0].snippets.push_back(std::move(s));

        summarized_up_to_ = batch_end;
        cascade(0);
    }
}

std::string TextDownsampler::render() const {
    std::string out;
    for (int i = MAX_MIPS - 1; i >= 0; --i) {
        for (const auto& snippet : levels_[i].snippets) {
            if (!out.empty()) out += ' ';
            out += snippet.text;
        }
    }
    return out;
}

std::string TextDownsampler::summarize(const std::string& passage, int level) {
    if (!llm_cb_) return {};

    auto prior = render_prior_context(level);

    std::string user_prompt =
        "<prior_context>" + prior + "</prior_context>\n"
        "<passage>" + passage + "</passage>\n"
        "Summarize only new elements not in prior_context. Short phrases, single line.";

    std::string full_prompt = std::string(SUMMARIZER_SYSTEM) + "\n\n" + user_prompt;

    try {
        auto result = llm_cb_(sanitize_utf8(full_prompt));
        // Strip whitespace
        auto start = result.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return {};
        auto end = result.find_last_not_of(" \t\n\r");
        return sanitize_utf8(result.substr(start, end - start + 1));
    } catch (const std::exception& e) {
        std::cerr << "  [downsampler] summarize failed: " << e.what() << "\n";
        return {};
    }
}

std::string TextDownsampler::render_prior_context(int target_level) const {
    std::string out;
    for (int i = target_level; i < MAX_MIPS; ++i) {
        for (const auto& snippet : levels_[i].snippets) {
            if (!out.empty()) out += ' ';
            out += snippet.text;
        }
    }
    return out.empty() ? "(none)" : out;
}

void TextDownsampler::cascade(int source_level) {
    auto& src = levels_[source_level];
    if (static_cast<int>(src.snippets.size()) <= src.max_snippets)
        return;

    int dest_level = source_level + 1;

    if (dest_level >= MAX_MIPS) {
        // Deepest level: circular buffer -- drop oldest
        while (static_cast<int>(src.snippets.size()) > src.max_snippets) {
            std::cerr << "  [downsampler] dropping oldest snippet from mip"
                      << source_level << "\n";
            src.snippets.erase(src.snippets.begin());
        }
        return;
    }

    auto& dest = levels_[dest_level];

    if (dest.snippets.empty()) {
        // Seed promotion: shift oldest verbatim (no LLM call)
        auto seed = std::move(src.snippets.front());
        src.snippets.erase(src.snippets.begin());
        seed.promoted = true;
        seed.source_mip = source_level;
        dest.snippets.push_back(std::move(seed));
        std::cerr << "  [downsampler] seed promotion mip" << source_level
                  << " -> mip" << dest_level << "\n";
    } else {
        // Merge oldest SNIPPETS_PER_PROMOTION snippets via LLM
        int count = std::min(SNIPPETS_PER_PROMOTION,
                             static_cast<int>(src.snippets.size()));
        std::vector<Snippet> to_merge(
            std::make_move_iterator(src.snippets.begin()),
            std::make_move_iterator(src.snippets.begin() + count));
        src.snippets.erase(src.snippets.begin(), src.snippets.begin() + count);

        std::string merged_text;
        for (const auto& s : to_merge) {
            if (!merged_text.empty()) merged_text += ' ';
            merged_text += s.text;
        }

        auto summary = summarize(merged_text, dest_level);
        if (summary.empty()) {
            // Rollback on failure
            src.snippets.insert(src.snippets.begin(),
                                std::make_move_iterator(to_merge.begin()),
                                std::make_move_iterator(to_merge.end()));
            std::cerr << "  [downsampler] cascade failed, rolled back\n";
            return;
        }

        Snippet meta;
        meta.text = std::move(summary);
        meta.turn_start = to_merge.front().turn_start;
        meta.turn_end = to_merge.back().turn_end;
        meta.timestamp = now_seconds();
        meta.source_mip = source_level;
        meta.merged_count = 0;
        for (const auto& s : to_merge) meta.merged_count += s.merged_count;
        dest.snippets.push_back(std::move(meta));
    }

    cascade(source_level);
    cascade(dest_level);
}

// -- Serialization --

nlohmann::json TextDownsampler::to_json() const {
    nlohmann::json j;
    j["summarized_up_to"] = summarized_up_to_;

    nlohmann::json levels_arr = nlohmann::json::array();
    for (const auto& level : levels_) {
        nlohmann::json lj;
        lj["max_snippets"] = level.max_snippets;

        nlohmann::json snippets_arr = nlohmann::json::array();
        for (const auto& s : level.snippets) {
            snippets_arr.push_back({
                {"text", s.text},
                {"turn_start", s.turn_start},
                {"turn_end", s.turn_end},
                {"timestamp", s.timestamp},
                {"promoted", s.promoted},
                {"source_mip", s.source_mip},
                {"merged_count", s.merged_count},
            });
        }
        lj["snippets"] = std::move(snippets_arr);
        levels_arr.push_back(std::move(lj));
    }
    j["levels"] = std::move(levels_arr);
    return j;
}

TextDownsampler TextDownsampler::from_json(const nlohmann::json& j) {
    TextDownsampler td;
    td.summarized_up_to_ = j.value("summarized_up_to", 0);

    if (j.contains("levels") && j["levels"].is_array()) {
        int i = 0;
        for (const auto& lj : j["levels"]) {
            if (i >= MAX_MIPS) break;
            td.levels_[i].max_snippets = lj.value("max_snippets",
                                                    td.levels_[i].max_snippets);
            td.levels_[i].snippets.clear();
            if (lj.contains("snippets") && lj["snippets"].is_array()) {
                for (const auto& sj : lj["snippets"]) {
                    Snippet s;
                    s.text = sanitize_utf8(sj.value("text", ""));
                    s.turn_start = sj.value("turn_start", 0);
                    s.turn_end = sj.value("turn_end", 0);
                    s.timestamp = sj.value("timestamp", 0.0);
                    s.promoted = sj.value("promoted", false);
                    s.source_mip = sj.value("source_mip", -1);
                    s.merged_count = sj.value("merged_count", 1);
                    td.levels_[i].snippets.push_back(std::move(s));
                }
            }
            ++i;
        }
    }
    return td;
}

} // namespace rhapsode
