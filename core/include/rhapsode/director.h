#pragma once
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "rhapsode/world_graph.h"
#include "rhapsode/validator.h"

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

using DirectorLLMCallback  = std::function<std::string(const std::string& prompt_json)>;

class Director {
public:
    explicit Director(WorldGraph& graph);

    void set_llm_callback(DirectorLLMCallback cb);
    void set_validator(Validator* v);
    DirectorOutput tick(int turn_index, const std::string& scene_context);

    /// JSON blob (serialized object) embedding `turn_index`, `scene_context`,
    /// `nodes`, and optional `graph_context_2hop` -- same payload the legacy
    /// director LLM received. Used by the merged narrator prompt.
    std::string focus_payload_json(int turn_index, const std::string& scene_context) const;

    /// Compact text payload for the narrator prompt: one-liner per node,
    /// Active + Foreshadowed only, with BFS-seeded focus IDs in the header.
    std::string focus_payload_text(int turn_index, const std::string& scene_context) const;

    /// Applies `transitions` / `new_nodes` from parsed JSON -- no LLM call.
    /// Extra keys such as `speech_turns` are ignored by the graph engine.
    DirectorOutput apply_planned_turn(int turn_index, const nlohmann::json& response);

private:
    WorldGraph& graph_;
    DirectorLLMCallback  llm_cb_;
    Validator* validator_ = nullptr;

    std::string    build_prompt(int turn_index, const std::string& scene_context) const;
    std::vector<Node> apply_transitions(const nlohmann::json& response, int turn_index);
    std::vector<Node> apply_new_nodes(const nlohmann::json& response, int turn_index,
                                      std::vector<Rejection>& rejections);
    DirectorOutput collect_context(std::vector<Node> expired) const;
};

} // namespace rhapsode
