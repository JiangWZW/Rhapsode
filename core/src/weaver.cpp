#include "rhapsode/weaver.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string_view>
#include <unordered_set>

namespace rhapsode {

namespace {

bool weave_log_verbose() {
    if (std::getenv("RHAPSODE_WEAVE_LOG")) return true;
    if (std::getenv("RHAPSODE_VERBOSE_LOG")) return true;
    return false;
}

std::uint64_t fnv1a64(std::string_view s) {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

std::string hex_u64(std::uint64_t v) {
    std::ostringstream os;
    os << std::hex << std::setfill('0') << std::setw(16) << v;
    return os.str();
}

void write_weave_artifact(int turn_index, const char* label,
                          const std::string& prompt,
                          const std::string& response,
                          const std::string& summary) {
    if (!weave_log_verbose()) return;

    const char* dir_env = std::getenv("RHAPSODE_WEAVE_LOG_DIR");
    std::filesystem::path dir = dir_env ? dir_env : "logs";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    std::ostringstream name;
    name << "weave-turn-" << turn_index << "-" << label << ".txt";
    std::filesystem::path path = dir / name.str();

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        log() << "  [weave] could not write artifact " << path.string() << "\n";
        return;
    }
    out << "turn=" << turn_index << " label=" << label << "\n"
        << "prompt_fnv=" << hex_u64(fnv1a64(prompt))
        << " prompt_chars=" << prompt.size() << "\n"
        << summary << "\n\n"
        << "=== PROMPT ===\n" << prompt << "\n\n"
        << "=== RESPONSE ===\n" << response << "\n";
    log() << "  [weave] artifact written: " << path.string() << "\n";
}

// -- Weaver::build_prompt helpers ---------------------------------------------

struct SubgraphSelection {
    // Own the sampled nodes. WorldGraph::all_nodes() returns a temporary vector,
    // so retaining pointers into it would leave every selection dangling.
    std::vector<Node> nodes;
    std::unordered_set<std::uint64_t> node_ids;
};

// Degree-biased sampling: take some low-degree nodes (needing connections) and
// some high-degree nodes (needing edge review), then fill remaining slots from
// the middle at random.  When the graph is small enough, take everything.
SubgraphSelection sample_degree_biased_subgraph(const WorldGraph& graph,
                                                std::mt19937& rng)
{
    constexpr size_t kMaxNodes  = 20;
    constexpr size_t kLowSlots  = 8;
    constexpr size_t kHighSlots = 8;

    auto all_nodes = graph.all_nodes(false);
    SubgraphSelection sel;

    if (all_nodes.size() <= kMaxNodes) {
        for (const auto& n : all_nodes) {
            sel.nodes.push_back(n);
            sel.node_ids.insert(n.id);
        }
        return sel;
    }

    struct NodeDeg { size_t idx; int deg; };
    std::vector<NodeDeg> nd;
    nd.reserve(all_nodes.size());
    for (size_t i = 0; i < all_nodes.size(); ++i)
        nd.push_back({i, graph.active_degree(all_nodes[i].id)});

    std::shuffle(nd.begin(), nd.end(), rng);
    std::stable_sort(nd.begin(), nd.end(),
        [](const NodeDeg& a, const NodeDeg& b) { return a.deg < b.deg; });

    auto try_add = [&](size_t idx) {
        if (sel.nodes.size() >= kMaxNodes) return;
        if (sel.node_ids.insert(all_nodes[idx].id).second)
            sel.nodes.push_back(all_nodes[idx]);
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
        std::shuffle(mid_idx.begin(), mid_idx.end(), rng);
        for (auto idx : mid_idx)
            try_add(idx);
    }

    return sel;
}

std::string assemble_weave_prompt(const WorldGraph& graph,
                                  const SubgraphSelection& sel,
                                  const std::string& scene_context)
{
    std::ostringstream os;
    os << "You are an editor reviewing a narrative knowledge graph.\n\n";

    if (!scene_context.empty())
        os << "### Scene context\n" << scene_context << "\n\n";

    os << "### Live nodes (current facts in the story world)\n";
    for (const auto& n : sel.nodes) {
        os << "[" << n.id << "] (" << to_string(n.state) << ") \""
           << n.fact << "\"";
        if (!n.entities.empty()) {
            os << "  entities={";
            for (size_t i = 0; i < n.entities.size(); ++i) {
                if (i) os << ", ";
                os << n.entities[i];
            }
            os << "}";
        }
        os << "\n";
    }

    os << "\n### Current edges\n";
    int edge_count = 0;
    for (const auto& ei : graph.all_edges()) {
        if (!ei.data.active) continue;
        if (!sel.node_ids.count(ei.from_id) || !sel.node_ids.count(ei.to_id))
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

}  // namespace

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

void Weaver::set_llm_callback(LLMCallback cb) { llm_cb_ = std::move(cb); }
void Weaver::set_local_llm_callback(LLMCallback cb) { local_llm_cb_ = std::move(cb); }
void Weaver::set_interval(int turns) { interval_ = turns > 0 ? turns : 1; }

bool Weaver::should_weave(int turn_index) const {
    return turn_index % interval_ == 0;
}

// ---------------------------------------------------------------------------
// Prompt builder
// ---------------------------------------------------------------------------

std::string Weaver::build_prompt(int turn_index,
                                 const std::string& scene_context) const {
    auto sel = sample_degree_biased_subgraph(graph_, rng_);

    // -- Debug log --
    log() << "  [weave] subgraph: " << sel.nodes.size() << "/"
          << graph_.all_nodes(false).size() << " nodes\n";
    log() << "  [weave] selected:";
    for (const auto& n : sel.nodes) {
        int d = graph_.active_degree(n.id);
        log() << " " << n.id << "(d" << d << ")";
    }
    log() << "\n";

    return assemble_weave_prompt(graph_, sel, scene_context);
}

// ---------------------------------------------------------------------------
// Parse LLM response and apply mutations
// ---------------------------------------------------------------------------

WeaveResult Weaver::parse_and_apply(const std::string& llm_response,
                                    int turn_index) {
    WeaveResult result;
    auto j = try_parse_json(llm_response);

    const bool parse_failed =
        !llm_response.empty() && j.empty();

    if (parse_failed) {
        log() << "  [weave] parse FAILED: JSON extraction failed — "
              << "response preview "
              << truncate_utf8_ellipsis(llm_response, 300) << "\n";
    }

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

    log() << "  [weave] parse: connect=" << connect_ops.size()
          << " disconnect=" << disconnect_ops.size()
          << " reweight=" << reweight_ops.size()
          << (parse_failed ? " (JSON salvage failed)" : "") << "\n";

    log() << "  [weave] LLM proposed: "
          << connect_ops.size() << " connect, "
          << disconnect_ops.size() << " disconnect, "
          << reweight_ops.size() << " reweight\n";

    for (auto& op : connect_ops) {
        bool ok = graph_.add_relation(op.from_id, op.to_id, op.weight, turn_index);
        log() << "  [weave]   + " << op.from_id << " -> " << op.to_id
              << "  w=" << op.weight
              << (ok ? "" : " (SKIP: already exists or bad id)")
              << "  \"" << op.reason << "\"\n";
        if (ok) result.connected.push_back(std::move(op));
    }

    for (auto& op : disconnect_ops) {
        bool ok = graph_.set_edge_active(op.from_id, op.to_id, false);
        log() << "  [weave]   - " << op.from_id << " -> " << op.to_id
              << (ok ? "" : " (SKIP: edge not found)")
              << "  \"" << op.reason << "\"\n";
        if (ok) result.disconnected.push_back(std::move(op));
    }

    for (auto& op : reweight_ops) {
        bool ok = graph_.set_edge_weight(op.from_id, op.to_id, op.weight);
        log() << "  [weave]   ~ " << op.from_id << " -> " << op.to_id
              << "  w=" << op.weight
              << (ok ? "" : " (SKIP: edge not found)")
              << "  \"" << op.reason << "\"\n";
        if (ok) result.reweighted.push_back(std::move(op));
    }

    result.analysis = analyze(graph_);

    log() << "  [weave] after: "
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
                                LLMCallback& cb, const char* label) {
    GraphAnalysis pre = analyze(graph_);

    if (pre.live_node_count < 2) {
        return {{}, {}, {}, pre};
    }

    if (!cb) {
        log() << "  [weave] no " << label << " LLM callback -- skipping\n";
        return {{}, {}, {}, pre};
    }

    log() << "  [weave] turn=" << turn_index << " label=" << label << "\n";
    log() << "  [weave] before: "
          << pre.live_node_count << " nodes, "
          << pre.active_edge_count << " active edges, "
          << pre.orphan_count << " orphans\n";

    auto prompt = build_prompt(turn_index, scene_context);
    log() << "  [weave] prompt_fnv=" << hex_u64(fnv1a64(prompt))
          << " prompt_chars=" << prompt.size()
          << " — calling " << label << " LLM...\n"
          << std::flush;

    if (weave_log_verbose()) {
        log() << "  [weave] --- PROMPT BEGIN ---\n"
              << prompt << "\n  [weave] --- PROMPT END ---\n"
              << std::flush;
    }

    std::string response;
    try {
        response = cb(sanitize_utf8(prompt));
    } catch (const std::exception& e) {
        log() << "  [weave] " << label << " LLM call FAILED: " << e.what() << "\n";
        write_weave_artifact(turn_index, label, prompt, "",
                             std::string("exception: ") + e.what());
        return {{}, {}, {}, pre};
    }

    log() << "  [weave] response_chars=" << response.size() << "\n";

    if (response.empty()) {
        log() << "  [weave] " << label
              << " LLM returned empty response "
              << "(see [local_llm:weave] lines above for HTTP details)\n";
        write_weave_artifact(turn_index, label, prompt, response,
                             "empty response from local LLM callback");
        return {{}, {}, {}, pre};
    }

    if (weave_log_verbose()) {
        log() << "  [weave] --- RESPONSE BEGIN ---\n"
              << response << "\n  [weave] --- RESPONSE END ---\n"
              << std::flush;
    }

    auto result = parse_and_apply(response, turn_index);

    std::ostringstream summary;
    summary << "response_chars=" << response.size()
            << " connect=" << result.connected.size()
            << " disconnect=" << result.disconnected.size()
            << " reweight=" << result.reweighted.size();
    write_weave_artifact(turn_index, label, prompt, response, summary.str());

    return result;
}

}  // namespace rhapsode
