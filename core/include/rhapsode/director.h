#pragma once
#include <functional>
#include <string>
#include <vector>
#include "rhapsode/world_graph.h"

namespace rhapsode {

struct DirectorOutput {
    std::vector<std::string> context_blocks;
    std::vector<Node>        newly_resolved;
    std::vector<Node>        new_nodes;
};

using DirectorLLMCallback  = std::function<std::string(const std::string& prompt_json)>;

class Director {
public:
    explicit Director(WorldGraph& graph);

    void set_llm_callback(DirectorLLMCallback cb);
    DirectorOutput tick(int turn_index, const std::string& scene_context);

private:
    WorldGraph& graph_;
    DirectorLLMCallback  llm_cb_;

    std::string    build_prompt(int turn_index, const std::string& scene_context) const;
    std::vector<Node> apply_transitions(const nlohmann::json& response, int turn_index);
    std::vector<Node> apply_new_nodes(const nlohmann::json& response, int turn_index);
    std::vector<Node> enforce_invariants(const std::vector<Node>& added, int turn_index);
    DirectorOutput collect_context(std::vector<Node> resolved) const;
};

} // namespace rhapsode
