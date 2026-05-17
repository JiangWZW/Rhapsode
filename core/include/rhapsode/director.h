#pragma once
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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

    /// JSON blob (serialized object) embedding `turn_index`, `scene_context`,
    /// `nodes`, and optional `graph_context_2hop` — same payload the legacy
    /// director LLM received. Used by the merged narrator prompt.
    std::string focus_payload_json(int turn_index, const std::string& scene_context) const;

    /// Applies `transitions` / `new_nodes` from parsed JSON — no LLM call.
    /// Extra keys such as `speech_turns` are ignored by the graph engine.
    DirectorOutput apply_planned_turn(int turn_index, const nlohmann::json& response);

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
