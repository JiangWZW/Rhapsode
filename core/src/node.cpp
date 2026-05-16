#include "rhapsode/node.h"
#include <algorithm>
#include <stdexcept>

namespace rhapsode {

std::string to_string(NodeState s) {
    switch (s) {
        case NodeState::Dormant:      return "dormant";
        case NodeState::Foreshadowed: return "foreshadowed";
        case NodeState::Active:       return "active";
        case NodeState::Resolved:     return "resolved";
    }
    throw std::runtime_error("Invalid NodeState");
}

NodeState node_state_from_string(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "dormant")      return NodeState::Dormant;
    if (lower == "foreshadowed") return NodeState::Foreshadowed;
    if (lower == "active")       return NodeState::Active;
    if (lower == "resolved")     return NodeState::Resolved;
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
        {"known_by",       n.known_by},
        {"related_to",     n.related_to},
        {"created_at",     n.created_at},
        {"resolved_at",    n.resolved_at}
    };
}

void from_json(const nlohmann::json& j, Node& n) {
    n.id             = j.value("id", std::uint64_t{0});
    n.fact           = j.value("fact", "");
    n.type           = j.value("type", "");
    n.state          = node_state_from_string(j.value("state", "dormant"));
    n.foreshadow_ctx = j.value("foreshadow_ctx", "");
    n.active_ctx     = j.value("active_ctx", "");
    n.entities       = j.value("entities", std::vector<std::string>{});
    n.known_by       = j.value("known_by", std::vector<std::string>{});
    n.related_to     = j.value("related_to", std::vector<std::uint64_t>{});
    n.created_at     = j.value("created_at", 0);
    n.resolved_at    = j.value("resolved_at", -1);
}

} // namespace rhapsode
