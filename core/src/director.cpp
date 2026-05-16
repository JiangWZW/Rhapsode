#include "rhapsode/director.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>

namespace rhapsode {

namespace {

std::string to_lower_copy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool shares_entity(const Node& a, const Node& b) {
    if (a.entities.empty() || b.entities.empty()) return false;
    std::set<std::string> as;
    for (const auto& e : a.entities) as.insert(to_lower_copy(e));
    for (const auto& e : b.entities) {
        if (as.count(to_lower_copy(e))) return true;
    }
    return false;
}

bool contains_any(const std::string& text, const std::vector<std::string>& needles) {
    std::string lower = to_lower_copy(text);
    for (const auto& n : needles) {
        if (lower.find(n) != std::string::npos) return true;
    }
    return false;
}

bool is_terminal_fact(const Node& n) {
    static const std::vector<std::string> kTerminal = {
        "dead", "dies", "killed", "destroyed", "incinerates", "obliterates", "collapsed"
    };
    return contains_any(n.fact, kTerminal);
}

std::string truncate(const std::string& s, size_t max_len = 60) {
    if (s.size() <= max_len) return s;
    return s.substr(0, max_len - 3) + "...";
}

void log_node_line(const Node& n, const char* prefix = "    ") {
    std::cerr << prefix << "[" << n.id << "] "
              << to_string(n.state) << " | " << n.type
              << " | \"" << truncate(n.fact) << "\"\n";
}

void log_prompt_summary(const nlohmann::json& prompt) {
    std::cerr << "\n  --- Director input summary ---\n";
    std::cerr << "  turn:          " << prompt.value("turn_index", -1) << "\n";

    auto sc = prompt.value("scene_context", "");
    std::cerr << "  scene_context: \"" << truncate(sc, 120) << "\"\n";

    if (prompt.contains("nodes") && prompt["nodes"].is_array()) {
        int dormant = 0, foreshadowed = 0, active = 0;
        for (const auto& n : prompt["nodes"]) {
            auto s = n.value("state", "");
            if (s == "active")        active++;
            else if (s == "foreshadowed") foreshadowed++;
            else if (s == "dormant")  dormant++;
        }
        std::cerr << "  graph nodes:   " << prompt["nodes"].size()
                  << " (active=" << active << " foreshadowed=" << foreshadowed
                  << " dormant=" << dormant << ")\n";
    }

    if (prompt.contains("graph_context_2hop") && prompt["graph_context_2hop"].is_array()) {
        std::cerr << "  2-hop context (" << prompt["graph_context_2hop"].size() << " nodes):\n";
        for (const auto& n : prompt["graph_context_2hop"]) {
            std::cerr << "    [" << n.value("id", 0) << "] "
                      << n.value("state", "?") << " | " << n.value("type", "?")
                      << " | \"" << truncate(n.value("fact", ""), 50) << "\"\n";
        }
    }

    std::cerr << "  prompt bytes:  " << prompt.dump().size() << "\n";
    std::cerr << "  ---\n";
}

void log_response_summary(const nlohmann::json& response, const WorldGraph& graph) {
    std::cerr << "\n  --- Director response summary ---\n";

    auto tr_it = response.find("transitions");
    if (tr_it != response.end() && tr_it->is_array() && !tr_it->empty()) {
        std::cerr << "  transitions (" << tr_it->size() << "):\n";
        for (const auto& t : *tr_it) {
            auto id = t.value("id", std::uint64_t{0});
            auto new_state = t.value("state", "?");
            const Node* n = graph.get_node(id);
            std::string fact_str = n ? ("\"" + truncate(n->fact, 50) + "\"")
                                     : "(unknown)";
            std::string old_state = n ? to_string(n->state) : "?";
            std::cerr << "    [" << id << "] " << old_state
                      << " -> " << new_state << "  " << fact_str << "\n";
        }
    } else {
        std::cerr << "  transitions: (none)\n";
    }

    auto nn_it = response.find("new_nodes");
    if (nn_it != response.end() && nn_it->is_array() && !nn_it->empty()) {
        std::cerr << "  new nodes (" << nn_it->size() << "):\n";
        for (const auto& n : *nn_it) {
            std::cerr << "    + " << n.value("state", "?")
                      << " | " << n.value("type", "?")
                      << " | \"" << truncate(n.value("fact", ""), 50) << "\"\n";
        }
    } else {
        std::cerr << "  new nodes: (none)\n";
    }

    std::cerr << "  ---\n";
}

}  // namespace

Director::Director(WorldGraph& graph) : graph_(graph) {}

void Director::set_llm_callback(DirectorLLMCallback cb) {
    llm_cb_ = std::move(cb);
}

DirectorOutput Director::tick(int turn_index, const std::string& scene_context) {
    if (!llm_cb_)
        throw std::runtime_error("No Director LLM callback registered");

    std::cerr << "  [1/5.a] Building director prompt...\n" << std::flush;
    auto prompt = build_prompt(turn_index, scene_context);

    {
        auto prompt_json = nlohmann::json::parse(prompt);
        log_prompt_summary(prompt_json);
    }

    std::cerr << "  [1/5.b] Calling director LLM...\n" << std::flush;
    auto raw = llm_cb_(prompt);

    std::cerr << "  [1/5.c] Parsing director response (" << raw.size() << " chars)...\n" << std::flush;
    nlohmann::json response;
    try {
        response = nlohmann::json::parse(raw);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Director JSON parse failed: ") + e.what()
                                 + " | raw[0:100]: " + raw.substr(0, 100));
    }

    log_response_summary(response, graph_);

    std::cerr << "  [1/5.d] Applying transitions...\n" << std::flush;
    auto resolved = apply_transitions(response, turn_index);

    std::cerr << "  [1/5.e] Applying new nodes...\n" << std::flush;
    auto added    = apply_new_nodes(response, turn_index);
    auto auto_resolved = enforce_invariants(added, turn_index);
    resolved.insert(resolved.end(), auto_resolved.begin(), auto_resolved.end());
    auto output   = collect_context(std::move(resolved));
    output.new_nodes = std::move(added);

    std::cerr << "\n  ===== WorldGraph after tick " << turn_index << " =====\n";
    auto all = graph_.all_nodes(false);
    std::sort(all.begin(), all.end(), [](const Node& a, const Node& b) {
        if (a.state != b.state) return a.state < b.state;
        return a.id < b.id;
    });
    for (const auto& n : all) {
        std::cerr << "    [" << n.id << "] " << to_string(n.state)
                  << " | " << n.type
                  << " | " << n.fact << "\n";
    }
    if (all.empty())
        std::cerr << "    (empty)\n";
    std::cerr << "  resolved: " << output.newly_resolved.size()
              << ", added: " << output.new_nodes.size()
              << ", graph total: " << graph_.size() << "\n";
    std::cerr << std::flush;

    return output;
}

std::string Director::build_prompt(int turn_index, const std::string& scene_context) const {
    nlohmann::json nodes_arr = nlohmann::json::array();
    auto all_nodes = graph_.all_nodes(false);
    for (const auto& node : all_nodes) {
        if (node.state != NodeState::Resolved) {
            nodes_arr.push_back(node);
        }
    }

    nlohmann::json prompt;
    prompt["turn_index"]    = turn_index;
    prompt["scene_context"] = scene_context;
    prompt["nodes"]         = std::move(nodes_arr);

    std::string scene_context_lower = to_lower_copy(scene_context);
    std::set<std::uint64_t> seed_ids;
    for (const auto& node : all_nodes) {
        if (node.state == NodeState::Resolved) continue;
        bool matched = false;
        for (const auto& e : node.entities) {
            if (!e.empty() && scene_context_lower.find(to_lower_copy(e)) != std::string::npos) {
                matched = true;
                break;
            }
        }
        if (!matched && !node.fact.empty() &&
            scene_context_lower.find(to_lower_copy(node.fact)) != std::string::npos) {
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
        for (size_t i = 0; i < std::min<size_t>(fallback.size(), 3); ++i) {
            if (fallback[i].state != NodeState::Resolved) seed_ids.insert(fallback[i].id);
        }
    }

    std::set<std::uint64_t> bfs_ids = seed_ids;
    for (auto id : seed_ids) {
        auto nearby = graph_.neighbors_within(id, 2, std::nullopt, true);
        bfs_ids.insert(nearby.begin(), nearby.end());
    }

    std::cerr << "  [director] BFS seeds (" << seed_ids.size()
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
            if (!node || node->state == NodeState::Resolved) continue;
            bfs_context.push_back(*node);
        }
        if (!bfs_context.empty()) {
            prompt["graph_context_2hop"] = std::move(bfs_context);
        }
    }

    return prompt.dump();
}

std::vector<Node> Director::apply_transitions(const nlohmann::json& response, int turn_index) {
    std::vector<Node> resolved;

    auto it = response.find("transitions");
    if (it == response.end() || !it->is_array())
        return resolved;

    for (const auto& entry : *it) {
        auto id        = entry.value("id", std::uint64_t{0});
        auto state_str = entry.value("state", "");
        if (id == 0 || state_str.empty())
            continue;

        Node* node = graph_.get_node(id);
        if (!node)
            continue;

        NodeState next_state = node_state_from_string(state_str);
        node->state = next_state;
        if (next_state == NodeState::Resolved) {
            graph_.mark_resolved(id, turn_index);
            resolved.push_back(*node);
        }
    }

    return resolved;
}

std::vector<Node> Director::apply_new_nodes(const nlohmann::json& response, int turn_index) {
    std::vector<Node> added;
    std::vector<Node> existing = graph_.all_nodes(false);
    auto it = response.find("new_nodes");
    if (it == response.end() || !it->is_array())
        return added;

    for (const auto& entry : *it) {
        Node node       = entry.get<Node>();
        node.id         = 0;
        node.created_at = turn_index;

        if (node.state == NodeState::Resolved && node.resolved_at < 0)
            node.resolved_at = turn_index;

        node.related_to.clear();
        Node& ref = graph_.add_node(std::move(node));

        for (const auto& prior : existing) {
            if (prior.id == ref.id || prior.state == NodeState::Resolved) continue;
            if (!shares_entity(ref, prior)) continue;
            if (graph_.add_relation(ref.id, prior.id, RelationKind::Related, 1.0f, turn_index)) {
                ref.related_to.push_back(prior.id);
            }
            graph_.add_relation(prior.id, ref.id, RelationKind::Related, 1.0f, turn_index);
        }
        existing.push_back(ref);
        added.push_back(ref);
    }
    return added;
}

std::vector<Node> Director::enforce_invariants(const std::vector<Node>& added, int turn_index) {
    std::vector<Node> auto_resolved;
    std::set<std::uint64_t> resolved_ids;

    for (const auto& fresh : added) {
        if (fresh.state == NodeState::Resolved) continue;
        if (fresh.related_to.empty()) continue;

        const bool fresh_terminal = is_terminal_fact(fresh);
        for (std::uint64_t related_id : fresh.related_to) {
            Node* prior = graph_.get_node(related_id);
            if (!prior) continue;
            if (prior->state == NodeState::Resolved) continue;
            if (resolved_ids.count(prior->id)) continue;
            if (!shares_entity(fresh, *prior)) continue;

            bool should_resolve = false;
            RelationKind relation = RelationKind::Related;

            if (fresh_terminal) {
                should_resolve = true;
                relation = RelationKind::Contradicts;
            } else if (fresh.state == NodeState::Active &&
                       prior->state == NodeState::Active &&
                       fresh.type == prior->type &&
                       fresh.fact != prior->fact) {
                should_resolve = true;
                relation = RelationKind::Supersedes;
            }

            if (!should_resolve) continue;

            graph_.mark_resolved(prior->id, turn_index);
            resolved_ids.insert(prior->id);
            graph_.add_relation(fresh.id, prior->id, relation, 1.0f, turn_index);

            Node snapshot = *prior;
            auto_resolved.push_back(snapshot);

            std::cerr << "  [invariant] auto-resolved [" << prior->id << "] by [" << fresh.id
                      << "] relation=" << to_string(relation)
                      << " fact=\"" << prior->fact << "\"\n";
        }
    }
    return auto_resolved;
}

DirectorOutput Director::collect_context(std::vector<Node> resolved) const {
    DirectorOutput output;
    output.newly_resolved = std::move(resolved);

    graph_.for_each([&](const Node& node) {
        if (node.state == NodeState::Foreshadowed && !node.foreshadow_ctx.empty())
            output.context_blocks.push_back(node.foreshadow_ctx);
        else if (node.state == NodeState::Active && !node.active_ctx.empty())
            output.context_blocks.push_back(node.active_ctx);
    });

    return output;
}

} // namespace rhapsode
