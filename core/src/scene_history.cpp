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

}  // namespace

void append_history_message(std::vector<SceneMessage>& history,
                            SceneMessage message) {
    if (message.timestamp.empty()) message.timestamp = utc_now_iso8601();
    message.content = sanitize_utf8(std::move(message.content));
    history.push_back(std::move(message));
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
    std::vector<std::string> keywords;
    std::string current;
    for (const char character : str::to_lower(query)) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            if (!current.empty()) {
                keywords.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += character;
        }
    }
    if (!current.empty()) keywords.push_back(std::move(current));

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

}  // namespace rhapsode
