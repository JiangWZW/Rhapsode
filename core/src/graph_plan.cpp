#include "rhapsode/graph_plan.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"
#include <algorithm>

namespace rhapsode {

namespace {

void log_response_summary(const nlohmann::json& response, const WorldGraph& graph) {
    log() << "\n  --- Graph plan summary ---\n";

    auto tr_it = response.find("transitions");
    if (tr_it != response.end() && tr_it->is_array() && !tr_it->empty()) {
        log() << "  transitions (" << tr_it->size() << "):\n";
        for (const auto& t : *tr_it) {
            auto id = json_number<std::uint64_t>(t, "id", 0);
            auto new_state = t.value("state", "?");
            const Node* n = graph.get_node(id);
            std::string fact_str = n ? ("\"" + truncate_utf8_ellipsis(n->fact, 50) + "\"")
                                     : "(unknown)";
            std::string old_state = n ? to_string(n->state) : "?";
            log() << "    [" << id << "] " << old_state
                  << " -> " << new_state << "  " << fact_str << "\n";
        }
    } else {
        log() << "  transitions: (none)\n";
    }

    auto nn_it = response.find("new_nodes");
    if (nn_it != response.end() && nn_it->is_array() && !nn_it->empty()) {
        log() << "  new nodes (" << nn_it->size() << "):\n";
        for (const auto& n : *nn_it) {
            log() << "    + " << n.value("state", "?")
                  << " | " << n.value("type", "?")
                  << " | \"" << truncate_utf8_ellipsis(n.value("fact", ""), 50) << "\"\n";
        }
    } else {
        log() << "  new nodes: (none)\n";
    }

    log() << "  ---\n";
}

}  // namespace

GraphPlanResult apply_graph_plan(
    WorldGraph& graph, int turn_index, const nlohmann::json& response) {
    log_response_summary(response, graph);

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
    graph.for_each([&](const Node& node) {
        if (node.state == NodeState::Foreshadowed && !node.foreshadow_ctx.empty())
            output.context_blocks.push_back(node.foreshadow_ctx);
        else if (node.state == NodeState::Active && !node.active_ctx.empty())
            output.context_blocks.push_back(node.active_ctx);
    });

    log() << "\n  ===== WorldGraph after apply " << turn_index << " =====\n";
    auto all = graph.all_nodes(false);
    std::sort(all.begin(), all.end(), [](const Node& a, const Node& b) {
        if (a.state != b.state) return a.state < b.state;
        return a.id < b.id;
    });
    for (const auto& n : all) {
        log() << "    [" << n.id << "] " << to_string(n.state)
              << " | " << n.type
              << " | " << n.fact << "\n";
    }
    if (all.empty())
        log() << "    (empty)\n";
    log() << "  expired: " << output.newly_expired.size()
          << ", added: " << output.new_nodes.size()
          << ", graph total: " << graph.size() << "\n";
    log() << std::flush;

    return output;
}

}  // namespace rhapsode
