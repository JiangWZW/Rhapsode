#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"
#include <stdexcept>
#include <algorithm>
#include <set>

namespace rhapsode {

namespace {

void log_node_line(const Node& n, const char* prefix = "    ") {
    log() << prefix << "[" << n.id << "] "
          << to_string(n.state) << " | " << n.type
          << " | \"" << truncate_utf8_ellipsis(n.fact, 60) << "\"\n";
}

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

void Director::set_validator(Validator* v) {
    validator_ = v;
}

std::string Director::focus_payload_json(int turn_index, const std::string& scene_context) const {
    return build_prompt(turn_index, scene_context);
}

std::string Director::build_prompt__world_graph_context(int turn_index,
                                                   const std::string& capped_prev_turns_text) const {
    (void)turn_index;
    auto all_nodes = graph_.all_nodes(false);

    // BFS seeding: entity-match against recent transcript, fallback to recency
    std::string scene_context_lower = str::to_lower(capped_prev_turns_text);
    std::set<std::uint64_t> seed_ids;
    for (const auto& node : all_nodes) {
        bool matched = false;
        for (const auto& e : node.entities) {
            if (!e.empty() && scene_context_lower.find(str::to_lower(e)) != std::string::npos) {
                matched = true;
                break;
            }
        }
        if (!matched && !node.fact.empty() &&
            scene_context_lower.find(str::to_lower(node.fact)) != std::string::npos) {
            matched = true;
        }
        if (matched) seed_ids.insert(node.id);
    }

    if (seed_ids.empty()) {
        std::vector<Node> fallback = all_nodes;
        std::sort(fallback.begin(), fallback.end(), [](const Node& a, const Node& b) {
            return a.created_at > b.created_at;
        });
        for (size_t i = 0; i < std::min<size_t>(fallback.size(), 3); ++i)
            seed_ids.insert(fallback[i].id);
    }

    std::set<std::uint64_t> bfs_ids = seed_ids;
    for (auto id : seed_ids) {
        auto nearby = graph_.neighbors_within(id, 2);
        bfs_ids.insert(nearby.begin(), nearby.end());
    }

    // Build compact text output: Active + Foreshadowed only
    std::string header = "[focus:";
    for (auto id : seed_ids)
        header += " " + std::to_string(id);
    header += "]";

    std::string body;
    for (auto id : bfs_ids) {
        const Node* n = graph_.get_node(id);
        if (!n) continue;
        if (n->state != NodeState::Active && n->state != NodeState::Foreshadowed)
            continue;
        if (n->valid_until != -1)
            continue;

        const std::string& ctx = (n->state == NodeState::Active)
            ? n->active_ctx : n->foreshadow_ctx;

        body += "[" + std::to_string(n->id) + "] "
              + to_string(n->state) + " " + n->type
              + " \"" + truncate_utf8_ellipsis(n->fact, 80) + "\"";
        if (!ctx.empty())
            body += " -- " + truncate_utf8_ellipsis(ctx, 100);
        body += "\n";
    }

    return header + "\n" + body;
}

DirectorOutput Director::apply_planned_turn(int turn_index, const nlohmann::json& response) {
    log_response_summary(response, graph_);

    log() << "  [graph] Applying transitions...\n" << std::flush;
    auto expired = apply_transitions(response, turn_index);

    log() << "  [graph] Applying new nodes...\n" << std::flush;
    std::vector<Rejection> rejections;
    auto added    = apply_new_nodes(response, turn_index, rejections);
    auto output   = collect_context(std::move(expired));
    output.new_nodes = std::move(added);
    output.rejections = std::move(rejections);

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

std::string Director::build_prompt(int turn_index, const std::string& scene_context) const {
    nlohmann::json nodes_arr = nlohmann::json::array();
    auto all_nodes = graph_.all_nodes(false);
    for (const auto& node : all_nodes)
        nodes_arr.push_back(node);

    nlohmann::json prompt;
    prompt["turn_index"]    = turn_index;
    prompt["scene_context"] = scene_context;
    prompt["nodes"]         = std::move(nodes_arr);

    std::string scene_context_lower = str::to_lower(scene_context);
    std::set<std::uint64_t> seed_ids;
    for (const auto& node : all_nodes) {
        bool matched = false;
        for (const auto& e : node.entities) {
            if (!e.empty() && scene_context_lower.find(str::to_lower(e)) != std::string::npos) {
                matched = true;
                break;
            }
        }
        if (!matched && !node.fact.empty() &&
            scene_context_lower.find(str::to_lower(node.fact)) != std::string::npos) {
            matched = true;
        }
        if (matched) seed_ids.insert(node.id);
    }

    bool used_fallback = seed_ids.empty();
    if (used_fallback) {
        std::vector<Node> fallback = all_nodes;
        std::sort(fallback.begin(), fallback.end(), [](const Node& a, const Node& b) {
            return a.created_at > b.created_at;
        });
        for (size_t i = 0; i < std::min<size_t>(fallback.size(), 3); ++i)
            seed_ids.insert(fallback[i].id);
    }

    std::set<std::uint64_t> bfs_ids = seed_ids;
    for (auto id : seed_ids) {
        auto nearby = graph_.neighbors_within(id, 2);
        bfs_ids.insert(nearby.begin(), nearby.end());
    }

    log() << "  [director] BFS seeds (" << seed_ids.size()
          << (used_fallback ? ", recency fallback" : ", entity-matched")
          << ") -> " << bfs_ids.size() << " context nodes:\n";
    for (auto id : seed_ids) {
        const Node* n = graph_.get_node(id);
        if (n) log_node_line(*n, "    seed ");
    }

    if (!bfs_ids.empty()) {
        nlohmann::json bfs_context = nlohmann::json::array();
        for (auto id : bfs_ids) {
            const Node* node = graph_.get_node(id);
            if (!node || node->valid_until != -1) continue;
            bfs_context.push_back(*node);
        }
        if (!bfs_context.empty()) {
            prompt["graph_context_2hop"] = std::move(bfs_context);
        }
    }

    return prompt.dump();
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

std::vector<Node> Director::apply_new_nodes(const nlohmann::json& response, int turn_index,
                                             std::vector<Rejection>& rejections) {
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

        if (validator_) {
            auto verdict = validator_->check(node);
            if (!verdict.accepted) {
                log() << "  [validator] REJECTED: \"" << node.fact
                      << "\" -- " << verdict.reason << "\n";
                rejections.push_back({node.fact, verdict.reason});
                continue;
            }
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
