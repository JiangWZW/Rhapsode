#pragma once
#include <vector>
#include <optional>
#include "rhapsode/scene_message.h"

namespace rhapsode {

class History {
public:
    void append(SceneMessage msg);
    std::vector<SceneMessage> snapshot(std::optional<size_t> n = std::nullopt) const;
    size_t size() const;
    void truncate(size_t new_size);
    void clear();
    /// Remove messages whose metadata["turn"] is >= min_turn (no-op if key missing).
    void drop_from_turn(int min_turn);
    const std::vector<SceneMessage>& messages() const;

private:
    std::vector<SceneMessage> messages_;
};

void to_json(nlohmann::json& j, const History& h);
void from_json(const nlohmann::json& j, History& h);

} // namespace rhapsode
