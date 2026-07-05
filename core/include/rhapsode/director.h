#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "rhapsode/world_graph.h"

namespace rhapsode {

struct Rejection {
    std::string fact;
    std::string reason;
};

struct DirectorOutput {
    std::vector<std::string> context_blocks;
    std::vector<Node>        newly_expired;
    std::vector<Node>        new_nodes;
    std::vector<Rejection>   rejections;
};

class Director {
public:
    explicit Director(WorldGraph& graph);

    /// Applies `transitions` / `new_nodes` from parsed JSON -- no LLM call.
    /// Extra keys such as `speech_turns` are ignored by the graph engine.
    DirectorOutput apply_planned_turn(int turn_index, const nlohmann::json& response);

private:
    WorldGraph& graph_;

    std::vector<Node> apply_transitions(const nlohmann::json& response, int turn_index);
    std::vector<Node> apply_new_nodes(const nlohmann::json& response, int turn_index,
                                      std::vector<Rejection>& rejections);
    DirectorOutput collect_context(std::vector<Node> expired) const;
};

} // namespace rhapsode
