#pragma once

#include <optional>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "rhapsode/scene_message.h"

namespace rhapsode {

void append_history_message(std::vector<SceneMessage>& history,
                            SceneMessage message);
std::vector<SceneMessage> snapshot_history(
    const std::vector<SceneMessage>& history,
    std::optional<std::size_t> count = std::nullopt);
void truncate_history(std::vector<SceneMessage>& history, std::size_t new_size);
void drop_history_from_turn(std::vector<SceneMessage>& history, int min_turn);
std::vector<SceneMessage> history_from_json(const nlohmann::json& value);

}  // namespace rhapsode
