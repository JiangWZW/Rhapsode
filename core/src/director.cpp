#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
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

std::string truncate(const std::string& s, size_t max_len = 60) {
    if (s.size() <= max_len) return s;
    size_t pos = max_len - 3;
    while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
        --pos;
    return s.substr(0, pos) + "...";
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
            auto id = json_number<std::uint64_t>(t, "id", 0);
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

void Director::set_validator(Validator* v) {
    validator_ = v;
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
    auto raw = llm_cb_(sanitize_utf8(prompt));

    std::cerr << "  [1/5.c] Parsing director response (" << raw.size() << " chars)...\n" << std::flush;
    nlohmann::json response;
    try {
        response = nlohmann::json::parse(raw);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Director JSON parse failed: ") + e.what()
                                 + " | raw[0:100]: " + raw.substr(0, 100));
    }

    return apply_planned_turn(turn_index, response);
}

std::string Director::focus_payload_json(int turn_index, const std::string& scene_context) const {
    return build_prompt(turn_index, scene_context);
}

std::string Director::focus_payload_text(int turn_index, const std::string& scene_context) const {
    auto all_nodes = graph_.all_nodes(false);

    // BFS seeding: entity-match against scene_context, fallback to recency
    std::string scene_context_lower = to_lower_copy(scene_context);
    std::set<std::uint64_t> seed_ids;
    for (const auto& node : all_nodes) {
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
              + " \"" + truncate(n->fact, 80) + "\"";
        if (!ctx.empty())
            body += " -- " + truncate(ctx, 100);
        body += "\n";
    }

    return header + "\n" + body;
}

DirectorOutput Director::apply_planned_turn(int turn_index, const nlohmann::json& response) {
    log_response_summary(response, graph_);

    std::cerr << "  [graph] Applying transitions...\n" << std::flush;
    auto expired = apply_transitions(response, turn_index);

    std::cerr << "  [graph] Applying new nodes...\n" << std::flush;
    std::vector<Rejection> rejections;
    auto added    = apply_new_nodes(response, turn_index, rejections);
    auto output   = collect_context(std::move(expired));
    output.new_nodes = std::move(added);
    output.rejections = std::move(rejections);

    std::cerr << "\n  ===== WorldGraph after apply " << turn_index << " =====\n";
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
    std::cerr << "  expired: " << output.newly_expired.size()
              << ", added: " << output.new_nodes.size()
              << ", graph total: " << graph_.size() << "\n";
    std::cerr << std::flush;

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

    std::string scene_context_lower = to_lower_copy(scene_context);
    std::set<std::uint64_t> seed_ids;
    for (const auto& node : all_nodes) {
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
        for (size_t i = 0; i < std::min<size_t>(fallback.size(), 3); ++i)
            seed_ids.insert(fallback[i].id);
    }

    std::set<std::uint64_t> bfs_ids = seed_ids;
    for (auto id : seed_ids) {
        auto nearby = graph_.neighbors_within(id, 2);
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

        auto lower_state = to_lower_copy(state_str);
        if (lower_state == "resolved") {
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
            auto raw_state = to_lower_copy(entry.value("state", ""));
            if (raw_state == "resolved")
                node.valid_until = turn_index;
        }

        if (validator_) {
            auto verdict = validator_->check(node);
            if (!verdict.accepted) {
                std::cerr << "  [validator] REJECTED: \"" << node.fact
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
