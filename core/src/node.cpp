#include "rhapsode/node.h"
#include "rhapsode/json_util.h"
#include <algorithm>
#include <stdexcept>

namespace rhapsode {

std::string to_string(NodeState s) {
    switch (s) {
        case NodeState::Dormant:      return "dormant";
        case NodeState::Foreshadowed: return "foreshadowed";
        case NodeState::Active:       return "active";
    }
    throw std::runtime_error("Invalid NodeState");
}

NodeState node_state_from_string(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "dormant")      return NodeState::Dormant;
    if (lower == "foreshadowed") return NodeState::Foreshadowed;
    if (lower == "active")       return NodeState::Active;
    if (lower == "resolved")     return NodeState::Active;  // migration
    throw std::runtime_error("Invalid node state string: " + s);
}

void to_json(nlohmann::json& j, const Node& n) {
    j = nlohmann::json{
        {"id",             n.id},
        {"fact",           n.fact},
        {"type",           n.type},
        {"state",          to_string(n.state)},
        {"foreshadow_ctx", n.foreshadow_ctx},
        {"active_ctx",     n.active_ctx},
        {"entities",       n.entities},
        {"related_to",     n.related_to},
        {"created_at",     n.created_at},
        {"valid_until",    n.valid_until}
    };
    if (!n.trigger.empty())      j["trigger"]      = n.trigger;
    if (!n.arc_position.empty()) j["arc_position"] = n.arc_position;
}

void from_json(const nlohmann::json& j, Node& n) {
    n.id             = json_number<std::uint64_t>(j, "id", 0);
    n.fact           = sanitize_utf8(j.value("fact", ""));
    n.type           = j.value("type", "");
    n.state          = node_state_from_string(j.value("state", "dormant"));
    n.foreshadow_ctx = sanitize_utf8(j.value("foreshadow_ctx", ""));
    n.active_ctx     = sanitize_utf8(j.value("active_ctx", ""));
    n.entities       = j.value("entities", std::vector<std::string>{});
    n.trigger        = sanitize_utf8(j.value("trigger", ""));
    n.arc_position   = j.value("arc_position", "");
    n.related_to     = j.value("related_to", std::vector<std::uint64_t>{});
    n.created_at     = json_number<int>(j, "created_at", 0);
    // Migration: prefer valid_until, fall back to resolved_at from old saves
    n.valid_until    = json_number<int>(j, "valid_until",
                           json_number<int>(j, "resolved_at", -1));
    // Perception routing (not persisted; only meaningful on freshly parsed
    // narrator new_nodes).
    n.audience       = j.value("audience", std::vector<std::string>{});
}

} // namespace rhapsode
