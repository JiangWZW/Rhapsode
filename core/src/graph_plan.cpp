#include "rhapsode/graph_plan.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

namespace rhapsode {

GraphPlanResult apply_graph_plan(
    WorldGraph& graph, int turn_index, const nlohmann::json& response) {
    log() << "  [graph] Applying transitions...\n" << std::flush;
    std::vector<Node> expired;
    auto transitions = response.find("transitions");
    if (transitions != response.end() && transitions->is_array()) {
        for (const auto& entry : *transitions) {
            auto id = json_number<std::uint64_t>(entry, "id", 0);
            auto state_str = entry.value("state", "");
            if (id == 0 || state_str.empty()) continue;

            Node* node = graph.get_node(id);
            if (!node) continue;
            if (node_state_means_resolved(state_str)) {
                node->state = NodeState::Active;
                graph.set_valid_until(id, turn_index);
                expired.push_back(*node);
            } else {
                node->state = node_state_from_string(state_str);
            }
        }
    }

    log() << "  [graph] Applying new nodes...\n" << std::flush;
    std::vector<Node> added;
    auto new_nodes = response.find("new_nodes");
    if (new_nodes != response.end() && new_nodes->is_array()) {
        for (const auto& entry : *new_nodes) {
            Node node = entry.get<Node>();
            node.id = 0;
            node.created_at = turn_index;
            if (node.valid_until < 0) {
                auto raw_state = str::to_lower(entry.value("state", ""));
                if (node_state_means_resolved(raw_state))
                    node.valid_until = turn_index;
            }
            Node& ref = graph.add_node_chained(std::move(node), turn_index);
            added.push_back(ref);
        }
    }

    GraphPlanResult output;
    output.newly_expired = std::move(expired);
    output.new_nodes = std::move(added);

    log() << "  [graph] expired=" << output.newly_expired.size()
          << " added=" << output.new_nodes.size()
          << " total=" << graph.size() << "\n" << std::flush;

    return output;
}

}  // namespace rhapsode
