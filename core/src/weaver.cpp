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

std::vector<WeaveOp> parse_weave_operations(const nlohmann::json& value) {
    std::vector<WeaveOp> operations;
    if (!value.is_array()) return operations;
    for (const auto& element : value) {
        if (!element.is_object()) continue;
        WeaveOp operation;
        operation.from_id = element.value("from", std::uint64_t{0});
        operation.to_id = element.value("to", std::uint64_t{0});
        operation.weight = element.value("weight", 1.0f);
        operation.reason = element.value("reason", "");
        if (operation.from_id && operation.to_id)
            operations.push_back(std::move(operation));
    }
    return operations;
}

void apply_connections(WorldGraph& graph, std::vector<WeaveOp> operations,
                       int turn_index, std::vector<WeaveOp>& applied) {
    for (auto& operation : operations) {
        const bool accepted = graph.add_relation(
            operation.from_id, operation.to_id, operation.weight, turn_index);
        log() << "  [weave]   + " << operation.from_id << " -> "
              << operation.to_id << "  w=" << operation.weight
              << (accepted ? "" : " (SKIP: already exists or bad id)")
              << "  \"" << operation.reason << "\"\n";
        if (accepted) applied.push_back(std::move(operation));
    }
}

void apply_disconnections(WorldGraph& graph, std::vector<WeaveOp> operations,
                          std::vector<WeaveOp>& applied) {
    for (auto& operation : operations) {
        const bool accepted = graph.set_edge_active(
            operation.from_id, operation.to_id, false);
        log() << "  [weave]   - " << operation.from_id << " -> "
              << operation.to_id
              << (accepted ? "" : " (SKIP: edge not found)")
              << "  \"" << operation.reason << "\"\n";
        if (accepted) applied.push_back(std::move(operation));
    }
}

void apply_reweights(WorldGraph& graph, std::vector<WeaveOp> operations,
                     std::vector<WeaveOp>& applied) {
    for (auto& operation : operations) {
        const bool accepted = graph.set_edge_weight(
            operation.from_id, operation.to_id, operation.weight);
        log() << "  [weave]   ~ " << operation.from_id << " -> "
              << operation.to_id << "  w=" << operation.weight
              << (accepted ? "" : " (SKIP: edge not found)")
              << "  \"" << operation.reason << "\"\n";
        if (accepted) applied.push_back(std::move(operation));
    }
}

WeaveResult empty_weave_result(const GraphAnalysis& analysis) {
    WeaveResult result;
    result.analysis = analysis;
    return result;
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

Weaver::Weaver(Weaver&& other) noexcept
    : llm_cb_(std::move(other.llm_cb_)),
      interval_(other.interval_),
      active_(other.active_),
      rng_(std::move(other.rng_)),
      expiry_stop_(other.expiry_stop_.load(std::memory_order_relaxed)),
      expiry_queue_(std::move(other.expiry_queue_)) {}

Weaver& Weaver::operator=(Weaver&& other) noexcept {
    if (this == &other) return *this;
    llm_cb_ = std::move(other.llm_cb_);
    interval_ = other.interval_;
    active_ = other.active_;
    rng_ = std::move(other.rng_);
    expiry_stop_.store(
        other.expiry_stop_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    expiry_queue_ = std::move(other.expiry_queue_);
    return *this;
}

void Weaver::set_llm_callback(LLMCallback cb) {
    llm_cb_ = std::move(cb);
    active_ = true;
}

void Weaver::set_interval(int turns) {
    interval_ = turns > 0 ? turns : 1;
    active_ = true;
}

bool Weaver::should_weave(int turn_index) const {
    return turn_index % interval_ == 0;
}

// ---------------------------------------------------------------------------
// Prompt builder
// ---------------------------------------------------------------------------

std::string Weaver::build_prompt(
    const WorldGraph& graph, const std::string& scene_context) const {
    auto sel = sample_degree_biased_subgraph(graph, rng_);

    // -- Debug log --
    log() << "  [weave] subgraph: " << sel.nodes.size() << "/"
          << graph.all_nodes(false).size() << " nodes\n";
    log() << "  [weave] selected:";
    for (const auto& n : sel.nodes) {
        int d = graph.active_degree(n.id);
        log() << " " << n.id << "(d" << d << ")";
    }
    log() << "\n";

    return assemble_weave_prompt(graph, sel, scene_context);
}

// ---------------------------------------------------------------------------
// Parse LLM response and apply mutations
// ---------------------------------------------------------------------------

WeaveResult Weaver::parse_and_apply(WorldGraph& graph,
                                    const std::string& llm_response,
                                    int turn_index) {
    WeaveResult result;
    auto j = try_parse_json(llm_response);

    const bool parse_failed =
        !llm_response.empty() && j.empty();

    if (parse_failed) {
        log_warn("weave") << "parse FAILED: JSON extraction failed — "
              << "response preview "
              << truncate_utf8_ellipsis(llm_response, 300) << "\n";
    }

    auto connect_ops = parse_weave_operations(
        j.value("connect", nlohmann::json::array()));
    auto disconnect_ops = parse_weave_operations(
        j.value("disconnect", nlohmann::json::array()));
    auto reweight_ops = parse_weave_operations(
        j.value("reweight", nlohmann::json::array()));

    log() << "  [weave] parse: connect=" << connect_ops.size()
          << " disconnect=" << disconnect_ops.size()
          << " reweight=" << reweight_ops.size()
          << (parse_failed ? " (JSON salvage failed)" : "") << "\n";

    log() << "  [weave] LLM proposed: "
          << connect_ops.size() << " connect, "
          << disconnect_ops.size() << " disconnect, "
          << reweight_ops.size() << " reweight\n";

    apply_connections(
        graph, std::move(connect_ops), turn_index, result.connected);
    apply_disconnections(
        graph, std::move(disconnect_ops), result.disconnected);
    apply_reweights(graph, std::move(reweight_ops), result.reweighted);

    result.analysis = analyze(graph);

    log() << "  [weave] after: "
          << result.analysis.live_node_count << " nodes, "
          << result.analysis.active_edge_count << " active edges, "
          << result.analysis.orphan_count << " orphans\n";

    return result;
}

// ---------------------------------------------------------------------------
// Main entry points
// ---------------------------------------------------------------------------

WeaveResult Weaver::weave(
    WorldGraph& graph, int turn_index, const std::string& scene_context) {
    return weave_impl(graph, turn_index, scene_context);
}

WeaveResult Weaver::weave_impl(
    WorldGraph& graph, int turn_index, const std::string& scene_context) {
    GraphAnalysis pre = analyze(graph);

    if (pre.live_node_count < 2) {
        return empty_weave_result(pre);
    }

    if (!llm_cb_) {
        log() << "  [weave] no LLM callback -- skipping\n";
        return empty_weave_result(pre);
    }

    log() << "  [weave] turn=" << turn_index << "\n";
    log() << "  [weave] before: "
          << pre.live_node_count << " nodes, "
          << pre.active_edge_count << " active edges, "
          << pre.orphan_count << " orphans\n";

    auto prompt = build_prompt(graph, scene_context);
    log() << "  [weave] prompt_fnv=" << hex_u64(fnv1a64(prompt))
          << " prompt_chars=" << prompt.size()
          << " — calling LLM...\n"
          << std::flush;

    if (weave_log_verbose()) {
        log() << "  [weave] --- PROMPT BEGIN ---\n"
              << prompt << "\n  [weave] --- PROMPT END ---\n"
              << std::flush;
    }

    std::string response;
    try {
        response = llm_cb_(sanitize_utf8(prompt));
    } catch (const std::exception& e) {
        log_warn("weave") << "LLM call FAILED: " << e.what() << "\n" << std::flush;
        write_weave_artifact(turn_index, "weave", prompt, "",
                             std::string("exception: ") + e.what());
        return empty_weave_result(pre);
    }

    log() << "  [weave] response_chars=" << response.size() << "\n";

    if (response.empty()) {
        log_warn("weave") << "LLM returned empty response\n" << std::flush;
        write_weave_artifact(turn_index, "weave", prompt, response,
                             "empty response");
        return empty_weave_result(pre);
    }

    if (weave_log_verbose()) {
        log() << "  [weave] --- RESPONSE BEGIN ---\n"
              << response << "\n  [weave] --- RESPONSE END ---\n"
              << std::flush;
    }

    auto result = parse_and_apply(graph, response, turn_index);

    std::ostringstream summary;
    summary << "response_chars=" << response.size()
            << " connect=" << result.connected.size()
            << " disconnect=" << result.disconnected.size()
            << " reweight=" << result.reweighted.size();
    write_weave_artifact(turn_index, "weave", prompt, response, summary.str());

    return result;
}

}  // namespace rhapsode
