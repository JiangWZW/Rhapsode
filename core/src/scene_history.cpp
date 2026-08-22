#include "rhapsode/scene_history.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "rhapsode/json_util.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

std::string utc_now_iso8601() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm fields{};
#ifdef _WIN32
    gmtime_s(&fields, &time);
#else
    gmtime_r(&time, &fields);
#endif
    std::ostringstream output;
    output << std::put_time(&fields, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

std::vector<std::string> split_query_terms(const std::string& query) {
    std::vector<std::string> terms;
    std::string current;
    for (const char character : str::to_lower(query)) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            if (!current.empty()) {
                terms.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += character;
        }
    }
    if (!current.empty()) terms.push_back(std::move(current));
    return terms;
}

}  // namespace

void append_history_message(std::vector<SceneMessage>& history,
                            SceneMessage message) {
    if (message.timestamp.empty()) message.timestamp = utc_now_iso8601();
    message.content = sanitize_utf8(std::move(message.content));
    history.push_back(std::move(message));
}

void append_lifecycle_note(SceneData& scene, const std::string& kind,
                           const std::string& content) {
    SceneMessage message;
    message.role = Role::System;
    message.content = content;
    message.metadata["scene_kind"] = kind;
    append_history_message(scene.history, std::move(message));
}

std::vector<SceneMessage> snapshot_history(
    const std::vector<SceneMessage>& history,
    std::optional<std::size_t> count) {
    if (!count || *count >= history.size()) return history;
    return std::vector<SceneMessage>(
        history.end() - static_cast<std::ptrdiff_t>(*count), history.end());
}

void truncate_history(std::vector<SceneMessage>& history, std::size_t new_size) {
    if (new_size < history.size()) history.resize(new_size);
}

void drop_history_from_turn(std::vector<SceneMessage>& history, int min_turn) {
    history.erase(
        std::remove_if(history.begin(), history.end(),
                       [min_turn](const SceneMessage& message) {
                           if (!message.metadata.contains("turn")) return false;
                           return message.metadata["turn"].get<int>() >= min_turn;
                       }),
        history.end());
}

std::vector<SceneMessage> history_from_json(const nlohmann::json& value) {
    std::vector<SceneMessage> history;
    history.reserve(value.size());
    for (const auto& item : value)
        append_history_message(history, item.get<SceneMessage>());
    return history;
}

std::string query_scene_history(const std::vector<SceneMessage>& history,
                                const std::string& query) {
    const std::vector<std::string> keywords = split_query_terms(query);

    std::vector<const SceneMessage*> matches;
    for (const auto& message : history) {
        const std::string text = str::to_lower(message.content);
        if (std::any_of(keywords.begin(), keywords.end(),
                        [&](const auto& keyword) {
                            return text.find(keyword) != std::string::npos;
                        })) {
            matches.push_back(&message);
        }
    }
    std::reverse(matches.begin(), matches.end());
    if (matches.size() > 10) matches.resize(10);

    const auto role_name = [](Role role) {
        switch (role) {
            case Role::User: return "user";
            case Role::Assistant: return "assistant";
            case Role::System: return "system";
        }
        return "user";
    };
    nlohmann::json snippets = nlohmann::json::array();
    for (const auto* message : matches) {
        snippets.push_back({
            {"role", role_name(message->role)},
            {"text", truncate_utf8(message->content, 400)}});
    }
    return nlohmann::json{{"snippets", std::move(snippets)}}.dump();
}

namespace {

std::optional<int> integer_metadata(const SceneMessage& message,
                                    const char* key) {
    if (!message.metadata.is_object()) return std::nullopt;
    const auto it = message.metadata.find(key);
    if (it == message.metadata.end() || !it->is_number_integer())
        return std::nullopt;
    return it->get<int>();
}

std::string string_metadata(const SceneMessage& message, const char* key) {
    if (!message.metadata.is_object()) return {};
    const auto it = message.metadata.find(key);
    return it != message.metadata.end() && it->is_string()
        ? it->get<std::string>() : std::string{};
}

std::string transcript_kind(const SceneMessage& message) {
    const std::string stored = string_metadata(message, "scene_kind");
    if (!stored.empty()) return stored;
    if (message.role == Role::User) return "player";
    if (message.role == Role::System) return "system";
    return string_metadata(message, "speaker").empty()
        ? "narrator" : "character";
}

std::string transcript_speaker(const SceneMessage& message,
                               const std::string& kind) {
    const std::string stored = string_metadata(message, "speaker");
    if (!stored.empty()) return stored;
    if (kind == "director_cue") return "Director";
    if (kind == "fork") return "Fork";
    if (kind == "merge") return "Merge";
    if (message.role == Role::User) return "Player";
    if (message.role == Role::System) return "System";
    return kind == "character" ? "Character" : "Narrator";
}

struct IndexedTranscriptSpan {
    TranscriptSpan span;
    std::size_t source_order = 0;
    bool structured_order = false;
};

IndexedTranscriptSpan make_transcript_span(
    const SceneData& scene, const SceneMessage& message,
    const std::string& channel, std::size_t channel_index,
    std::size_t source_order) {
    IndexedTranscriptSpan indexed;
    indexed.source_order = source_order;
    indexed.span.scene_id = scene.scene_id;
    indexed.span.message = message;
    indexed.span.exact_content = message.content;
    indexed.span.kind = transcript_kind(message);
    indexed.span.speaker = transcript_speaker(message, indexed.span.kind);

    const auto turn = integer_metadata(message, "turn");
    const auto ordinal = integer_metadata(message, "turn_ordinal");
    indexed.structured_order = turn.has_value() && ordinal.has_value();
    indexed.span.legacy_order = !indexed.structured_order;
    indexed.span.turn = turn.value_or(-1);
    indexed.span.ordinal = ordinal.value_or(-1);

    indexed.span.reference = string_metadata(message, "message_ref");
    indexed.span.stable_reference = !indexed.span.reference.empty();
    if (indexed.span.reference.empty()) {
        if (indexed.structured_order) {
            indexed.span.reference = scene.scene_id + ":turn:" +
                std::to_string(*turn) + ":" + indexed.span.kind + ":" +
                std::to_string(*ordinal);
        } else {
            indexed.span.reference = scene.scene_id + ":legacy:" + channel +
                ":" + std::to_string(channel_index);
        }
    }
    return indexed;
}

}  // namespace

std::vector<TranscriptSpan> attributed_transcript(
    const SceneData& scene, std::optional<std::size_t> count) {
    std::vector<IndexedTranscriptSpan> indexed;
    indexed.reserve(scene.history.size() + scene.dialogue.size());
    std::size_t source_order = 0;
    for (std::size_t i = 0; i < scene.history.size(); ++i) {
        indexed.push_back(make_transcript_span(
            scene, scene.history[i], "history", i, source_order++));
    }
    for (std::size_t i = 0; i < scene.dialogue.size(); ++i) {
        indexed.push_back(make_transcript_span(
            scene, scene.dialogue[i], "dialogue", i, source_order++));
    }

    const bool all_structured = std::all_of(
        indexed.begin(), indexed.end(),
        [](const IndexedTranscriptSpan& item) {
            return item.structured_order;
        });
    std::stable_sort(indexed.begin(), indexed.end(),
        [all_structured](const IndexedTranscriptSpan& left,
                         const IndexedTranscriptSpan& right) {
            if (all_structured) {
                if (left.span.turn != right.span.turn)
                    return left.span.turn < right.span.turn;
                if (left.span.ordinal != right.span.ordinal)
                    return left.span.ordinal < right.span.ordinal;
                return left.source_order < right.source_order;
            }
            if (left.span.message.timestamp != right.span.message.timestamp)
                return left.span.message.timestamp < right.span.message.timestamp;
            if (left.structured_order != right.structured_order)
                return left.structured_order;
            if (left.structured_order) {
                if (left.span.turn != right.span.turn)
                    return left.span.turn < right.span.turn;
                if (left.span.ordinal != right.span.ordinal)
                    return left.span.ordinal < right.span.ordinal;
            }
            return left.source_order < right.source_order;
        });

    const std::size_t start = count && *count < indexed.size()
        ? indexed.size() - *count : 0;
    std::vector<TranscriptSpan> result;
    result.reserve(indexed.size() - start);
    for (std::size_t i = start; i < indexed.size(); ++i)
        result.push_back(std::move(indexed[i].span));
    return result;
}

std::string query_attributed_transcript(const SceneData& scene,
                                        const std::string& query,
                                        std::size_t max_results) {
    const std::vector<std::string> keywords = split_query_terms(query);
    const auto timeline = attributed_transcript(scene);
    std::vector<const TranscriptSpan*> matches;
    for (auto it = timeline.rbegin(); it != timeline.rend(); ++it) {
        const std::string searchable = str::to_lower(
            it->speaker + " " + it->kind + " " + it->exact_content);
        const bool matched = keywords.empty() ||
            std::any_of(keywords.begin(), keywords.end(),
                [&](const auto& keyword) {
                    return searchable.find(keyword) != std::string::npos;
                });
        if (matched) matches.push_back(&*it);
        if (matches.size() >= max_results) break;
    }

    nlohmann::json spans = nlohmann::json::array();
    for (const auto* span : matches) {
        spans.push_back({
            {"reference", span->reference},
            {"scene_id", span->scene_id},
            {"turn", span->turn},
            {"ordinal", span->ordinal},
            {"kind", span->kind},
            {"speaker", span->speaker},
            {"text", span->exact_content},
            {"retrieval_reason", keywords.empty() ? "recent" : "lexical"},
            {"stable_reference", span->stable_reference},
            {"legacy_order", span->legacy_order},
        });
    }
    return nlohmann::json{{"spans", std::move(spans)}}.dump();
}

std::string format_visible_transcript(const SceneData& scene) {
    std::ostringstream out;
    for (const auto& span : attributed_transcript(scene)) {
        if (span.kind == "director_cue") continue;
        if (span.message.role == Role::System &&
            span.kind != "fork" && span.kind != "merge")
            continue;
        const std::string content = str::trim(span.message.content);
        if (content.empty()) continue;
        out << "[" << span.speaker << "]\n" << content << "\n\n";
    }
    return out.str();
}

}  // namespace rhapsode
