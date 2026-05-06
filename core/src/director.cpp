#include "rhapsode/director.h"
#include <stdexcept>

namespace rhapsode {

Director::Director(NodePool& pool) : pool_(pool) {}

void Director::set_llm_callback(DirectorLLMCallback cb) {
    llm_cb_ = std::move(cb);
}

void Director::set_retrieval_callback(RetrievalCallback cb) {
    retrieval_cb_ = std::move(cb);
}

DirectorOutput Director::tick(int turn_index, const std::string& scene_context) {
    if (!llm_cb_)
        throw std::runtime_error("No Director LLM callback registered");

    auto prompt   = build_prompt(turn_index, scene_context);
    auto raw      = llm_cb_(prompt);
    auto response = nlohmann::json::parse(raw);

    auto resolved = apply_transitions(response, turn_index);
    apply_new_nodes(response, turn_index);
    return collect_context(std::move(resolved));
}

std::string Director::build_prompt(int turn_index, const std::string& scene_context) const {
    nlohmann::json nodes_arr = nlohmann::json::array();
    pool_.for_each([&](const Node& node) {
        if (node.state != NodeState::Resolved)
            nodes_arr.push_back(node);
    });

    nlohmann::json prompt;
    prompt["turn_index"]    = turn_index;
    prompt["scene_context"] = scene_context;
    prompt["nodes"]         = std::move(nodes_arr);

    if (retrieval_cb_) {
        auto retrieved_json = retrieval_cb_(scene_context);
        auto retrieved      = nlohmann::json::parse(retrieved_json);
        if (retrieved.is_array() && !retrieved.empty())
            prompt["resolved_context"] = std::move(retrieved);
    }

    return prompt.dump();
}

std::vector<Node> Director::apply_transitions(const nlohmann::json& response, int turn_index) {
    std::vector<Node> resolved;

    auto it = response.find("transitions");
    if (it == response.end() || !it->is_array())
        return resolved;

    std::vector<std::uint64_t> to_remove;

    for (const auto& entry : *it) {
        auto id        = entry.value("id", std::uint64_t{0});
        auto state_str = entry.value("state", "");
        if (id == 0 || state_str.empty())
            continue;

        Node* node = pool_.get(id);
        if (!node)
            continue;

        node->state = node_state_from_string(state_str);
        if (node->state == NodeState::Resolved) {
            node->resolved_at = turn_index;
            resolved.push_back(*node);
            to_remove.push_back(id);
        }
    }

    for (auto id : to_remove)
        pool_.remove(id);

    return resolved;
}

void Director::apply_new_nodes(const nlohmann::json& response, int turn_index) {
    auto it = response.find("new_nodes");
    if (it == response.end() || !it->is_array())
        return;

    for (const auto& entry : *it) {
        Node node       = entry.get<Node>();
        node.id         = 0;
        node.created_at = turn_index;

        if (node.state == NodeState::Resolved && node.resolved_at < 0)
            node.resolved_at = turn_index;

        pool_.add(std::move(node));
    }
}

DirectorOutput Director::collect_context(std::vector<Node> resolved) const {
    DirectorOutput output;
    output.newly_resolved = std::move(resolved);

    pool_.for_each([&](const Node& node) {
        if (node.state == NodeState::Foreshadowed && !node.foreshadow_ctx.empty())
            output.context_blocks.push_back(node.foreshadow_ctx);
        else if (node.state == NodeState::Active && !node.active_ctx.empty())
            output.context_blocks.push_back(node.active_ctx);
    });

    return output;
}

} // namespace rhapsode
