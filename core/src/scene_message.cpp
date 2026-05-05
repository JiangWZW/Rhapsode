#include "rhapsode/scene_message.h"

namespace rhapsode {

void to_json(nlohmann::json& j, const SceneMessage& m) {
    j = nlohmann::json{
        {"role", m.role},
        {"content", m.content},
        {"timestamp", m.timestamp},
        {"metadata", m.metadata}
    };
}

void from_json(const nlohmann::json& j, SceneMessage& m) {
    j.at("role").get_to(m.role);
    j.at("content").get_to(m.content);
    if (j.contains("timestamp")) {
        j.at("timestamp").get_to(m.timestamp);
    }
    if (j.contains("metadata")) {
        m.metadata = j.at("metadata");
    } else {
        m.metadata = nlohmann::json::object();
    }
}

} // namespace rhapsode
