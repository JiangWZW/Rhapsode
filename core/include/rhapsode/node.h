#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace rhapsode {

enum class NodeState {
    Dormant,
    Foreshadowed,
    Active
};

struct Node {
    std::uint64_t id = 0;
    std::string fact;
    std::string type;
    NodeState state = NodeState::Dormant;
    std::string foreshadow_ctx;
    std::string active_ctx;
    std::vector<std::string> entities;
    std::string trigger;
    std::string arc_position;
    std::vector<std::uint64_t> related_to;
    int created_at = 0;
    int valid_until = -1;  // -1 = still valid; >0 = turn when superseded

    // How much this node presses (a Thought's "weight").  Not factual
    // confidence: it is reinforced when new related material touches it and
    // decays when untouched (set only by the background reflection pass).
    // Facts leave it at 0; only character minds use it.
    float weight = 0.0f;

    // Perception routing (transient): which characters perceive this fact.
    // Set by the narrator on new_nodes; consumed when routing the fact into
    // character minds.  NOT serialized into the world graph -- it is an
    // instruction, not world state.  Empty = a public beat (all present perceive).
    std::vector<std::string> audience;
};

std::string to_string(NodeState s);
NodeState node_state_from_string(const std::string& s);
/// True for "resolved" and common LLM shorthand "resolve".
bool node_state_means_resolved(const std::string& s);

void to_json(nlohmann::json& j, const Node& n);
void from_json(const nlohmann::json& j, Node& n);

} // namespace rhapsode
