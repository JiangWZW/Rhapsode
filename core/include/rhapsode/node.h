#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace rhapsode {

enum class NodeState {
    Dormant,
    Foreshadowed,
    Active,
    Resolved
};

struct Node {
    std::uint64_t id = 0;
    std::string fact;
    std::string type;
    NodeState state = NodeState::Dormant;
    std::string foreshadow_ctx;
    std::string active_ctx;
    std::vector<std::string> entities;
    std::vector<std::string> known_by;
    std::vector<std::uint64_t> related_to;
    int created_at = 0;
    int resolved_at = -1;
};

std::string to_string(NodeState s);
NodeState node_state_from_string(const std::string& s);

void to_json(nlohmann::json& j, const Node& n);
void from_json(const nlohmann::json& j, Node& n);

} // namespace rhapsode
