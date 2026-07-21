#include "rhapsode/scene_history.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "rhapsode/json_util.h"

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

}  // namespace rhapsode
