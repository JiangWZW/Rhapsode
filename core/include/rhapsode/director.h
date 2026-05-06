#pragma once
#include <functional>
#include <string>
#include <vector>
#include "rhapsode/node_pool.h"

namespace rhapsode {

struct DirectorOutput {
    std::vector<std::string> context_blocks;
    std::vector<Node>        newly_resolved;
};

using DirectorLLMCallback  = std::function<std::string(const std::string& prompt_json)>;
using RetrievalCallback    = std::function<std::string(const std::string& context_json)>;

class Director {
public:
    explicit Director(NodePool& pool);

    void set_llm_callback(DirectorLLMCallback cb);
    void set_retrieval_callback(RetrievalCallback cb);
    DirectorOutput tick(int turn_index, const std::string& scene_context);

private:
    NodePool& pool_;
    DirectorLLMCallback  llm_cb_;
    RetrievalCallback    retrieval_cb_;

    std::string    build_prompt(int turn_index, const std::string& scene_context) const;
    std::vector<Node> apply_transitions(const nlohmann::json& response, int turn_index);
    void           apply_new_nodes(const nlohmann::json& response, int turn_index);
    DirectorOutput collect_context(std::vector<Node> resolved) const;
};

} // namespace rhapsode
