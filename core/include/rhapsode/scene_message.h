#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace rhapsode {

enum class Role { System, User, Assistant };

NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::System,    "system"},
    {Role::User,      "user"},
    {Role::Assistant, "assistant"},
})

struct SceneMessage {
    Role role;
    std::string content;
    std::string timestamp;
    nlohmann::json metadata = nlohmann::json::object();
};

void to_json(nlohmann::json& j, const SceneMessage& m);
void from_json(const nlohmann::json& j, SceneMessage& m);

} // namespace rhapsode
