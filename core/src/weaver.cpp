#include "rhapsode/weaver.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_set>

namespace rhapsode {

// ---------------------------------------------------------------------------
// Free function: lightweight graph analysis
// ---------------------------------------------------------------------------

GraphAnalysis analyze(const WorldGraph& graph) {
    GraphAnalysis a;

    auto nodes = graph.all_nodes(false);
    a.live_node_count = static_cast<int>(nodes.size());

    int total_deg = 0;
    for (const auto& n : nodes) {
        int d = graph.active_degree(n.id);
        if (d == 0) ++a.orphan_count;
        total_deg += d;
    }
    a.active_edge_count = total_deg / 2;

    return a;
}

// ---------------------------------------------------------------------------
// Weaver
// ---------------------------------------------------------------------------

Weaver::Weaver(WorldGraph& graph) : graph_(graph) {}

void Weaver::set_llm_callback(WeaverLLMCallback cb) { llm_cb_ = std::move(cb); }
void Weaver::set_local_llm_callback(WeaverLLMCallback cb) { local_llm_cb_ = std::move(cb); }
void Weaver::set_interval(int turns) { interval_ = turns > 0 ? turns : 1; }

bool Weaver::should_weave(int turn_index) const {
    return turn_index % interval_ == 0;
}

// ---------------------------------------------------------------------------
// Prompt builder
// ---------------------------------------------------------------------------

std::string Weaver::build_prompt(int turn_index,
                                 const std::string& scene_context) const {
    constexpr size_t kMaxNodes  = 20;
    constexpr size_t kLowSlots  = 8;
    constexpr size_t kHighSlots = 8;

    auto all_nodes = graph_.all_nodes(false);

    // -- Degree-biased subgraph selection --
    std::vector<const Node*> selected;
    std::unordered_set<std::uint64_t> selected_ids;

    if (all_nodes.size() <= kMaxNodes) {
        for (const auto& n : all_nodes) {
            selected.push_back(&n);
            selected_ids.insert(n.id);
        }
    } else {
        struct NodeDeg { size_t idx; int deg; };
        std::vector<NodeDeg> nd;
        nd.reserve(all_nodes.size());
        for (size_t i = 0; i < all_nodes.size(); ++i)
            nd.push_back({i, graph_.active_degree(all_nodes[i].id)});

        std::shuffle(nd.begin(), nd.end(), rng_);
        std::stable_sort(nd.begin(), nd.end(),
            [](const NodeDeg& a, const NodeDeg& b) { return a.deg < b.deg; });

        auto try_add = [&](size_t idx) {
            if (selected.size() >= kMaxNodes) return;
            if (selected_ids.insert(all_nodes[idx].id).second)
                selected.push_back(&all_nodes[idx]);
        };

        for (size_t i = 0; i < std::min(nd.size(), kLowSlots); ++i)
            try_add(nd[i].idx);

        for (size_t i = 0; i < std::min(nd.size(), kHighSlots); ++i)
            try_add(nd[nd.size() - 1 - i].idx);

        size_t mid_lo = std::min(kLowSlots, nd.size());
        size_t mid_hi = nd.size() > kHighSlots ? nd.size() - kHighSlots : 0;
        if (mid_lo < mid_hi) {
            std::vector<size_t> mid_idx;
            mid_idx.reserve(mid_hi - mid_lo);
            for (size_t i = mid_lo; i < mid_hi; ++i)
                mid_idx.push_back(nd[i].idx);
            std::shuffle(mid_idx.begin(), mid_idx.end(), rng_);
            for (auto idx : mid_idx)
                try_add(idx);
        }
    }

    // -- Debug log --
    std::cerr << "  [weave] subgraph: " << selected.size() << "/"
              << all_nodes.size() << " nodes\n";
    std::cerr << "  [weave] selected:";
    for (const auto* n : selected) {
        int d = graph_.active_degree(n->id);
        std::cerr << " " << n->id << "(d" << d << ")";
    }
    std::cerr << "\n";

    // -- Build prompt --
    std::ostringstream os;
    os << "You are an editor reviewing a narrative knowledge graph.\n\n";

    if (!scene_context.empty())
        os << "### Scene context\n" << scene_context << "\n\n";

    os << "### Live nodes (current facts in the story world)\n";
    for (const auto* n : selected) {
        os << "[" << n->id << "] (" << to_string(n->state) << ") \""
           << n->fact << "\"";
        if (!n->entities.empty()) {
            os << "  entities={";
            for (size_t i = 0; i < n->entities.size(); ++i) {
                if (i) os << ", ";
                os << n->entities[i];
            }
            os << "}";
        }
        os << "\n";
    }

    os << "\n### Current edges\n";
    int edge_count = 0;
    for (const auto& ei : graph_.all_edges()) {
        if (!ei.data.active) continue;
        if (!selected_ids.count(ei.from_id) || !selected_ids.count(ei.to_id))
            continue;
        ++edge_count;
        os << ei.from_id << " -> " << ei.to_id
           << "  (weight=" << ei.data.weight << ")\n";
    }
    if (edge_count == 0) os << "(none)\n";

    os << "\n### Known issues\n"
       << "These nodes were selected by degree: low-degree nodes (needing "
          "connections) and high-degree nodes (needing edge review) are "
          "prioritized. You are seeing a partial view of the full graph. "
          "Existing edges come from entity-name overlap "
          "(shares_entity heuristic) and may be wrong or missing.\n";

    os << "\n### Task\n"
       << "Analyse the node CONTENT (not just entities). Output a JSON object "
          "with three arrays:\n"
       << "```\n"
       << "{\n"
       << "  \"connect\":    [{\"from\": <id>, \"to\": <id>, \"weight\": <float>, \"reason\": \"...\"}],\n"
       << "  \"disconnect\": [{\"from\": <id>, \"to\": <id>, \"reason\": \"...\"}],\n"
       << "  \"reweight\":   [{\"from\": <id>, \"to\": <id>, \"weight\": <float>, \"reason\": \"...\"}]\n"
       << "}\n"
       << "```\n"
       << "Weight guide: 0.3 = weak thematic, 0.6 = moderate, 1.0 = strong causal.\n"
       << "Only include operations that improve narrative coherence. "
          "Empty arrays are fine. Output ONLY the JSON object, no prose.\n";

    return os.str();
}

// ---------------------------------------------------------------------------
// Parse LLM response and apply mutations
// ---------------------------------------------------------------------------

WeaveResult Weaver::parse_and_apply(const std::string& llm_response,
                                    int turn_index) {
    WeaveResult result;
    auto j = try_parse_json(llm_response);

    auto parse_ops = [](const nlohmann::json& arr) {
        std::vector<WeaveOp> ops;
        if (!arr.is_array()) return ops;
        for (const auto& el : arr) {
            if (!el.is_object()) continue;
            WeaveOp op;
            op.from_id = el.value("from", std::uint64_t{0});
            op.to_id   = el.value("to",   std::uint64_t{0});
            op.weight  = el.value("weight", 1.0f);
            op.reason  = el.value("reason", "");
            if (op.from_id && op.to_id)
                ops.push_back(std::move(op));
        }
        return ops;
    };

    auto connect_ops    = parse_ops(j.value("connect",    nlohmann::json::array()));
    auto disconnect_ops = parse_ops(j.value("disconnect", nlohmann::json::array()));
    auto reweight_ops   = parse_ops(j.value("reweight",   nlohmann::json::array()));

    std::cerr << "  [weave] LLM proposed: "
              << connect_ops.size() << " connect, "
              << disconnect_ops.size() << " disconnect, "
              << reweight_ops.size() << " reweight\n";

    for (auto& op : connect_ops) {
        bool ok = graph_.add_relation(op.from_id, op.to_id, op.weight, turn_index);
        std::cerr << "  [weave]   + " << op.from_id << " -> " << op.to_id
                  << "  w=" << op.weight
                  << (ok ? "" : " (SKIP: already exists or bad id)")
                  << "  \"" << op.reason << "\"\n";
        if (ok) result.connected.push_back(std::move(op));
    }

    for (auto& op : disconnect_ops) {
        bool ok = graph_.set_edge_active(op.from_id, op.to_id, false);
        std::cerr << "  [weave]   - " << op.from_id << " -> " << op.to_id
                  << (ok ? "" : " (SKIP: edge not found)")
                  << "  \"" << op.reason << "\"\n";
        if (ok) result.disconnected.push_back(std::move(op));
    }

    for (auto& op : reweight_ops) {
        bool ok = graph_.set_edge_weight(op.from_id, op.to_id, op.weight);
        std::cerr << "  [weave]   ~ " << op.from_id << " -> " << op.to_id
                  << "  w=" << op.weight
                  << (ok ? "" : " (SKIP: edge not found)")
                  << "  \"" << op.reason << "\"\n";
        if (ok) result.reweighted.push_back(std::move(op));
    }

    result.analysis = analyze(graph_);

    std::cerr << "  [weave] after: "
              << result.analysis.live_node_count << " nodes, "
              << result.analysis.active_edge_count << " active edges, "
              << result.analysis.orphan_count << " orphans\n";

    return result;
}

// ---------------------------------------------------------------------------
// Main entry points
// ---------------------------------------------------------------------------

WeaveResult Weaver::weave(int turn_index, const std::string& scene_context) {
    return weave_impl(turn_index, scene_context, llm_cb_, "cloud");
}

WeaveResult Weaver::weave_local(int turn_index, const std::string& scene_context) {
    return weave_impl(turn_index, scene_context, local_llm_cb_, "local");
}

WeaveResult Weaver::weave_impl(int turn_index, const std::string& scene_context,
                                WeaverLLMCallback& cb, const char* label) {
    GraphAnalysis pre = analyze(graph_);

    if (pre.live_node_count < 2) {
        return {{}, {}, {}, pre};
    }

    if (!cb) {
        std::cerr << "  [weave] no " << label << " LLM callback -- skipping\n";
        return {{}, {}, {}, pre};
    }

    std::cerr << "  [weave] before: "
              << pre.live_node_count << " nodes, "
              << pre.active_edge_count << " active edges, "
              << pre.orphan_count << " orphans\n";

    auto prompt = build_prompt(turn_index, scene_context);
    std::cerr << "  [weave] prompt built (" << prompt.size()
              << " chars), calling " << label << " LLM...\n"
              << std::flush;

    std::string response;
    try {
        response = cb(sanitize_utf8(prompt));
    } catch (const std::exception& e) {
        std::cerr << "  [weave] " << label << " LLM call FAILED: " << e.what() << "\n";
        return {{}, {}, {}, pre};
    }

    if (response.empty()) {
        std::cerr << "  [weave] " << label << " LLM returned empty response\n";
        return {{}, {}, {}, pre};
    }

    std::cerr << "  [weave] " << label << " LLM response ("
              << response.size() << " chars):\n"
              << response << "\n";

    return parse_and_apply(response, turn_index);
}

// ---------------------------------------------------------------------------
// Entity-group expiry detector
// ---------------------------------------------------------------------------

void Weaver::rebuild_expiry_queue(
        const std::vector<std::string>& priority_entities) {
    expiry_queue_.clear();
    auto groups = graph_.entity_groups();

    std::set<std::string> prio_lower;
    for (const auto& e : priority_entities) {
        std::string lower = e;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        prio_lower.insert(std::move(lower));
    }

    // Partition into priority and non-priority groups; deduplicate.
    std::vector<std::vector<std::uint64_t>> priority_groups, normal_groups;
    std::set<std::vector<std::uint64_t>> seen;
    for (auto& [entity, ids] : groups) {
        if (ids.size() < 2) continue;
        auto canonical = ids;
        std::sort(canonical.begin(), canonical.end());
        if (!seen.insert(std::move(canonical)).second) continue;

        std::string lower = entity;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (prio_lower.count(lower))
            priority_groups.push_back(std::move(ids));
        else
            normal_groups.push_back(std::move(ids));
    }

    // drain_expiry_queue pops from the back, so priority goes last
    expiry_queue_ = std::move(normal_groups);
    expiry_queue_.insert(expiry_queue_.end(),
                         std::make_move_iterator(priority_groups.begin()),
                         std::make_move_iterator(priority_groups.end()));

    if (!expiry_queue_.empty())
        std::cerr << "  [expiry] queue rebuilt: "
                  << expiry_queue_.size() << " group(s), "
                  << priority_groups.size() << " priority\n";
}

std::vector<ExpiryOp> Weaver::drain_expiry_queue(int turn_index) {
    expiry_stop_.store(false, std::memory_order_relaxed);
    std::vector<ExpiryOp> all;

    while (!expiry_queue_.empty()
           && !expiry_stop_.load(std::memory_order_relaxed)) {
        auto ids = std::move(expiry_queue_.back());
        expiry_queue_.pop_back();

        std::vector<const Node*> live;
        for (auto id : ids) {
            const Node* n = graph_.get_node(id);
            if (n && n->valid_until == -1)
                live.push_back(n);
        }
        if (live.size() < 2) continue;

        auto expired = check_group(std::move(live), turn_index);
        all.insert(all.end(),
                   std::make_move_iterator(expired.begin()),
                   std::make_move_iterator(expired.end()));
    }
    return all;
}

void Weaver::stop_expiry_drain() {
    expiry_stop_.store(true, std::memory_order_relaxed);
}

bool Weaver::expiry_queue_empty() const {
    return expiry_queue_.empty();
}

std::vector<ExpiryOp> Weaver::check_group(std::vector<const Node*> live,
                                           int turn_index) {
    std::sort(live.begin(), live.end(),
              [](const Node* a, const Node* b) {
                  return a->created_at > b->created_at;
              });

    std::ostringstream os;
    os << "Active facts (newest first):\n";
    for (const auto* n : live)
        os << "- [" << n->id << "] (turn " << n->created_at
           << ") \"" << n->fact << "\"\n";

    os << "\nWhich of these facts are NO LONGER TRUE given the full set?\n"
          "A fact is superseded only if a newer fact makes it factually false.\n"
          "Story progression (A happened, then B happened) where both remain "
          "true is NOT supersession.\n"
          "For each superseded fact, state which newer fact supersedes it.\n"
          "Output ONLY a JSON object:\n"
          "{\"superseded\": [{\"id\": <old>, \"by\": <newer>}], \"reason\": \"...\"}\n"
          "If all are still true: {\"superseded\": [], \"reason\": \"all current\"}\n";

    if (!local_llm_cb_) {
        std::cerr << "  [expiry] no local LLM callback -- skipping group\n";
        return {};
    }

    std::string response;
    try {
        response = local_llm_cb_(sanitize_utf8(os.str()));
    } catch (const std::exception& e) {
        std::cerr << "  [expiry] LLM call failed: " << e.what() << "\n";
        return {};
    }

    std::unordered_set<std::uint64_t> valid_ids;
    for (const auto* n : live)
        valid_ids.insert(n->id);

    auto j = try_parse_json(response);
    auto reason = j.value("reason", std::string{});

    std::vector<ExpiryOp> expired;
    auto arr = j.value("superseded", nlohmann::json::array());
    if (!arr.is_array()) return expired;

    for (const auto& elem : arr) {
        std::uint64_t old_id = 0;
        int vu = turn_index;  // fallback: use current turn

        if (elem.is_object()) {
            old_id = json_number<std::uint64_t>(elem, "id", 0);
            auto by_id = json_number<std::uint64_t>(elem, "by", 0);
            if (by_id != 0) {
                const Node* by_node = graph_.get_node(by_id);
                if (by_node)
                    vu = by_node->created_at;
            }
        } else if (elem.is_number_integer()) {
            old_id = elem.get<std::uint64_t>();
        }

        if (old_id == 0 || !valid_ids.count(old_id)) continue;
        if (graph_.set_valid_until(old_id, vu))
            expired.push_back({old_id, reason});
    }

    if (!expired.empty())
        std::cerr << "  [expiry] " << expired.size()
                  << " fact(s) superseded: " << reason << "\n";

    return expired;
}

}  // namespace rhapsode
