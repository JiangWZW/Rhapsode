#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"
#include <algorithm>

namespace rhapsode {

namespace {

void log_response_summary(const nlohmann::json& response, const WorldGraph& graph) {
    log() << "\n  --- Director response summary ---\n";

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

Director::Director(WorldGraph& graph) : graph_(graph) {}

DirectorOutput Director::apply_planned_turn(int turn_index, const nlohmann::json& response) {
    log_response_summary(response, graph_);

    log() << "  [graph] Applying transitions...\n" << std::flush;
    auto expired = apply_transitions(response, turn_index);

    log() << "  [graph] Applying new nodes...\n" << std::flush;
    auto added    = apply_new_nodes(response, turn_index);
    auto output   = collect_context(std::move(expired));
    output.new_nodes = std::move(added);

    log() << "\n  ===== WorldGraph after apply " << turn_index << " =====\n";
    auto all = graph_.all_nodes(false);
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
          << ", graph total: " << graph_.size() << "\n";
    log() << std::flush;

    return output;
}

std::vector<Node> Director::apply_transitions(const nlohmann::json& response, int turn_index) {
    std::vector<Node> expired;

    auto it = response.find("transitions");
    if (it == response.end() || !it->is_array())
        return expired;

    for (const auto& entry : *it) {
        auto id        = json_number<std::uint64_t>(entry, "id", 0);
        auto state_str = entry.value("state", "");
        if (id == 0 || state_str.empty())
            continue;

        Node* node = graph_.get_node(id);
        if (!node)
            continue;

        if (node_state_means_resolved(state_str)) {
            node->state = NodeState::Active;
            graph_.set_valid_until(id, turn_index);
            expired.push_back(*node);
        } else {
            node->state = node_state_from_string(state_str);
        }
    }

    return expired;
}

std::vector<Node> Director::apply_new_nodes(const nlohmann::json& response,
                                            int turn_index) {
    std::vector<Node> added;
    auto it = response.find("new_nodes");
    if (it == response.end() || !it->is_array())
        return added;

    for (const auto& entry : *it) {
        Node node       = entry.get<Node>();
        node.id         = 0;
        node.created_at = turn_index;

        // Director LLM may create nodes as "resolved" — from_json maps to Active.
        // If valid_until was set from the LLM's resolved_at, keep it.
        // If not provided (still -1), check the raw state string.
        if (node.valid_until < 0) {
            auto raw_state = str::to_lower(entry.value("state", ""));
            if (node_state_means_resolved(raw_state))
                node.valid_until = turn_index;
        }

        Node& ref = graph_.add_node_chained(std::move(node), turn_index);
        added.push_back(ref);
    }
    return added;
}

DirectorOutput Director::collect_context(std::vector<Node> expired) const {
    DirectorOutput output;
    output.newly_expired = std::move(expired);

    graph_.for_each([&](const Node& node) {
        if (node.state == NodeState::Foreshadowed && !node.foreshadow_ctx.empty())
            output.context_blocks.push_back(node.foreshadow_ctx);
        else if (node.state == NodeState::Active && !node.active_ctx.empty())
            output.context_blocks.push_back(node.active_ctx);
    });

    return output;
}

} // namespace rhapsode
