#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "rhapsode/scene_message.h"

namespace rhapsode {

struct SceneData;

struct TranscriptSpan {
    std::string reference;
    std::string scene_id;
    int turn = -1;
    int ordinal = -1;
    std::string kind;
    std::string speaker;
    std::string exact_content;
    SceneMessage message;
    bool stable_reference = false;
    bool legacy_order = false;
};

void append_history_message(std::vector<SceneMessage>& history,
                            SceneMessage message);
std::vector<SceneMessage> snapshot_history(
    const std::vector<SceneMessage>& history,
    std::optional<std::size_t> count = std::nullopt);
void truncate_history(std::vector<SceneMessage>& history, std::size_t new_size);
void drop_history_from_turn(std::vector<SceneMessage>& history, int min_turn);
std::vector<SceneMessage> history_from_json(const nlohmann::json& value);
std::string query_scene_history(const std::vector<SceneMessage>& history,
                                const std::string& query);
std::vector<TranscriptSpan> attributed_transcript(
    const SceneData& scene,
    std::optional<std::size_t> count = std::nullopt);
std::string query_attributed_transcript(const SceneData& scene,
                                        const std::string& query,
                                        std::size_t max_results = 10);

}  // namespace rhapsode
