#include "rhapsode/text_downsampling.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <utility>

#include <nlohmann/json.hpp>

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"

namespace rhapsode {
namespace {

constexpr int kBatchSize = 3;
constexpr int kSnippetsPerPromotion = 2;
constexpr int kMaxMips = 3;

constexpr const char* kSummarizerSystem =
    "You are a precise narrative-state tracker. "
    "Output only the summary line.";

double now_seconds() {
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

const char* role_name(Role role) {
    switch (role) {
        case Role::User: return "user";
        case Role::Assistant: return "assistant";
        case Role::System: return "system";
    }
    return "unknown";
}

std::string render_prior_context(const DownsamplingState& state,
                                 int target_level) {
    std::string output;
    for (int index = target_level; index < kMaxMips; ++index) {
        for (const auto& snippet : state.levels[index].snippets) {
            if (!output.empty()) output += ' ';
            output += snippet.text;
        }
    }
    return output.empty() ? "(none)" : output;
}

std::string summarize(const DownsamplingState& state,
                      const std::string& passage, int level,
                      const LLMCallback& llm_callback) {
    if (!llm_callback) return {};

    const std::string prompt =
        std::string(kSummarizerSystem) + "\n\n" +
        "<prior_context>" + render_prior_context(state, level) +
        "</prior_context>\n<passage>" + passage + "</passage>\n" +
        "Summarize only new elements not in prior_context. Short phrases, "
        "single line.";
    try {
        const auto result = llm_callback(sanitize_utf8(prompt));
        const auto start = result.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return {};
        const auto end = result.find_last_not_of(" \t\n\r");
        return sanitize_utf8(result.substr(start, end - start + 1));
    } catch (const std::exception& error) {
        log() << "  [downsampler] summarize failed: " << error.what() << "\n";
        return {};
    }
}

void cascade(DownsamplingState& state, int source_level,
             const LLMCallback& llm_callback) {
    auto& source = state.levels[source_level];
    if (static_cast<int>(source.snippets.size()) <= source.max_snippets) return;

    const int destination_level = source_level + 1;
    if (destination_level >= kMaxMips) {
        while (static_cast<int>(source.snippets.size()) > source.max_snippets) {
            log() << "  [downsampler] dropping oldest snippet from mip"
                  << source_level << "\n";
            source.snippets.erase(source.snippets.begin());
        }
        return;
    }

    auto& destination = state.levels[destination_level];
    if (destination.snippets.empty()) {
        auto seed = std::move(source.snippets.front());
        source.snippets.erase(source.snippets.begin());
        seed.promoted = true;
        seed.source_mip = source_level;
        destination.snippets.push_back(std::move(seed));
        log() << "  [downsampler] seed promotion mip" << source_level
              << " -> mip" << destination_level << "\n";
    } else {
        const int count = std::min(
            kSnippetsPerPromotion, static_cast<int>(source.snippets.size()));
        std::vector<Snippet> to_merge(
            std::make_move_iterator(source.snippets.begin()),
            std::make_move_iterator(source.snippets.begin() + count));
        source.snippets.erase(
            source.snippets.begin(), source.snippets.begin() + count);

        std::string merged_text;
        for (const auto& snippet : to_merge) {
            if (!merged_text.empty()) merged_text += ' ';
            merged_text += snippet.text;
        }

        auto summary = summarize(
            state, merged_text, destination_level, llm_callback);
        if (summary.empty()) {
            source.snippets.insert(
                source.snippets.begin(),
                std::make_move_iterator(to_merge.begin()),
                std::make_move_iterator(to_merge.end()));
            log() << "  [downsampler] cascade failed, rolled back\n";
            return;
        }

        Snippet metadata;
        metadata.text = std::move(summary);
        metadata.turn_start = to_merge.front().turn_start;
        metadata.turn_end = to_merge.back().turn_end;
        metadata.timestamp = now_seconds();
        metadata.source_mip = source_level;
        metadata.merged_count = 0;
        for (const auto& snippet : to_merge)
            metadata.merged_count += snippet.merged_count;
        destination.snippets.push_back(std::move(metadata));
    }

    cascade(state, source_level, llm_callback);
    cascade(state, destination_level, llm_callback);
}

}  // namespace

void process_text_downsampling(DownsamplingState& state,
                               const std::vector<SceneMessage>& messages,
                               const LLMCallback& llm_callback,
                               int verbatim_tail) {
    const int total = static_cast<int>(messages.size());
    const int eligible_end = std::max(0, total - verbatim_tail);
    while (state.summarized_up_to + kBatchSize <= eligible_end) {
        const int batch_start = state.summarized_up_to;
        const int batch_end = batch_start + kBatchSize;

        std::string batch_text;
        for (int index = batch_start; index < batch_end; ++index) {
            if (!batch_text.empty()) batch_text += '\n';
            batch_text += std::string(role_name(messages[index].role)) + ": " +
                          messages[index].content;
        }

        auto summary = summarize(state, batch_text, 0, llm_callback);
        if (summary.empty()) break;

        Snippet snippet;
        snippet.text = std::move(summary);
        snippet.turn_start = batch_start;
        snippet.turn_end = batch_end - 1;
        snippet.timestamp = now_seconds();
        state.levels[0].snippets.push_back(std::move(snippet));
        state.summarized_up_to = batch_end;
        cascade(state, 0, llm_callback);
    }
}

std::string render_text_downsampling(const DownsamplingState& state) {
    std::string output;
    for (int index = kMaxMips - 1; index >= 0; --index) {
        for (const auto& snippet : state.levels[index].snippets) {
            if (!output.empty()) output += ' ';
            output += snippet.text;
        }
    }
    return output;
}

nlohmann::json downsampling_to_json(const DownsamplingState& state) {
    nlohmann::json value;
    value["summarized_up_to"] = state.summarized_up_to;
    nlohmann::json levels = nlohmann::json::array();
    for (const auto& level : state.levels) {
        nlohmann::json level_value;
        level_value["max_snippets"] = level.max_snippets;
        nlohmann::json snippets = nlohmann::json::array();
        for (const auto& snippet : level.snippets) {
            snippets.push_back({
                {"text", snippet.text},
                {"turn_start", snippet.turn_start},
                {"turn_end", snippet.turn_end},
                {"timestamp", snippet.timestamp},
                {"promoted", snippet.promoted},
                {"source_mip", snippet.source_mip},
                {"merged_count", snippet.merged_count},
            });
        }
        level_value["snippets"] = std::move(snippets);
        levels.push_back(std::move(level_value));
    }
    value["levels"] = std::move(levels);
    return value;
}

DownsamplingState downsampling_from_json(const nlohmann::json& value) {
    DownsamplingState state;
    state.summarized_up_to = value.value("summarized_up_to", 0);
    if (!value.contains("levels") || !value["levels"].is_array()) return state;

    int index = 0;
    for (const auto& level_value : value["levels"]) {
        if (index >= kMaxMips) break;
        auto& level = state.levels[index];
        level.max_snippets =
            level_value.value("max_snippets", level.max_snippets);
        level.snippets.clear();
        if (level_value.contains("snippets") &&
            level_value["snippets"].is_array()) {
            for (const auto& snippet_value : level_value["snippets"]) {
                Snippet snippet;
                snippet.text = sanitize_utf8(snippet_value.value("text", ""));
                snippet.turn_start = snippet_value.value("turn_start", 0);
                snippet.turn_end = snippet_value.value("turn_end", 0);
                snippet.timestamp = snippet_value.value("timestamp", 0.0);
                snippet.promoted = snippet_value.value("promoted", false);
                snippet.source_mip = snippet_value.value("source_mip", -1);
                snippet.merged_count = snippet_value.value("merged_count", 1);
                level.snippets.push_back(std::move(snippet));
            }
        }
        ++index;
    }
    return state;
}

}  // namespace rhapsode
