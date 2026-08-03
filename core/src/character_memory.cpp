#include "rhapsode/character_memory.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace rhapsode {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CharacterMemory::CharacterMemory(std::string name)
    : character_name_(std::move(name)) {
    ensure_bootstrap("");
}

// ---------------------------------------------------------------------------
// Subjective belief graph (beliefs_)
// ---------------------------------------------------------------------------

namespace {

// Case-insensitive identity test for two entity strings.  Exact equality only.
// The narrator is the sole identity authority: it emits canonical subjects
// (exact Cast names, "Player"), so the same referent always arrives as the same
// string.  Substring/containment is deliberately NOT used -- it both falsely
// merges distinct names that share a token ("Ash" vs "Ashenmoor", any
// "...captain...") and misses true aliases with no shared substring.  Identity
// is decided once at the source, never re-guessed here.
bool entity_matches(const std::string& a_lower, const std::string& b_lower) {
    return !a_lower.empty() && a_lower == b_lower;
}

}  // namespace

std::uint64_t CharacterMemory::seed_belief(const std::string& fact,
                                           const std::vector<std::string>& entities,
                                           int created_at,
                                           float weight,
                                           const std::string& type) {
    if (fact.empty()) return 0;
    Node n;
    n.fact       = sanitize_utf8(fact);
    n.type       = type;
    n.state      = NodeState::Active;
    n.entities   = entities;
    n.created_at = created_at;
    n.valid_until = -1;
    n.weight     = weight;
    return beliefs_.add_node_chained(std::move(n), created_at).id;
}

bool CharacterMemory::expire_intention(std::uint64_t node_id,
                                       int valid_until) {
    const Node* node = beliefs_.get_node(node_id);
    if (!node || node->type != "intention") return false;
    return beliefs_.set_valid_until(node_id, valid_until);
}

void CharacterMemory::link_tension(std::uint64_t a_id, std::uint64_t b_id, int turn) {
    if (a_id == 0 || b_id == 0 || a_id == b_id) return;
    if (!beliefs_.has_node(a_id) || !beliefs_.has_node(b_id)) return;
    // If the pair is already linked (e.g. an authored "chain" edge between two
    // same-subject seeds), add_relation refuses the duplicate; upgrade that edge
    // to a tension so the held contradiction surfaces in render_thoughts.
    if (!beliefs_.add_relation(a_id, b_id, 1.0f, turn, "tension"))
        beliefs_.set_edge_kind(a_id, b_id, "tension");
}

namespace {

// -- render_thoughts helpers --------------------------------------------------

struct ThoughtRenderMode {
    bool no_charge = false;
    static ThoughtRenderMode from_env() {
        ThoughtRenderMode m;
        m.no_charge = std::getenv("RHAPSODE_BASELINE_NO_CHARGE") != nullptr;
        return m;
    }
};

struct ThoughtChain {
    std::string subject;
    std::vector<const Node*> nodes;
    float peak = 0.0f;
};

std::string short_fact(const std::string& f) {
    if (f.size() <= 80) return f;
    return truncate_utf8(f, 77) + "...";
}

std::unordered_set<std::uint64_t> collect_tension_node_ids(const WorldGraph& beliefs) {
    std::unordered_set<std::uint64_t> in_tension;
    for (const auto& e : beliefs.all_edges()) {
        if (e.data.kind != "tension") continue;
        in_tension.insert(e.from_id);
        in_tension.insert(e.to_id);
    }
    return in_tension;
}

std::vector<ThoughtChain> group_and_order_thought_chains(
    const std::vector<const Node*>& thoughts, bool no_charge)
{
    std::unordered_map<std::string, std::vector<const Node*>> chains;
    for (const auto* n : thoughts) {
        std::string key = n->entities.empty() ? std::string("(myself)")
                                              : n->entities.front();
        chains[key].push_back(n);
    }
    std::vector<ThoughtChain> ordered;
    ordered.reserve(chains.size());
    for (auto& [subj, ns] : chains) {
        std::sort(ns.begin(), ns.end(),
                  [](const Node* a, const Node* b) { return a->created_at < b->created_at; });
        float peak = 0.0f;
        for (const auto* n : ns) peak = std::max(peak, n->weight);
        ordered.push_back({subj, std::move(ns), peak});
    }
    if (no_charge)
        // Control arm: most-recently-formed chains lead, weight ignored.
        std::sort(ordered.begin(), ordered.end(),
                  [](const ThoughtChain& a, const ThoughtChain& b) {
                      return a.nodes.back()->created_at > b.nodes.back()->created_at;
                  });
    else
        // Most-pressing chains lead.
        std::sort(ordered.begin(), ordered.end(),
                  [](const ThoughtChain& a, const ThoughtChain& b) { return a.peak > b.peak; });
    return ordered;
}

void append_thought_chains(std::ostringstream& os,
                           const std::vector<ThoughtChain>& ordered,
                           const ThoughtRenderMode& mode,
                           const std::unordered_set<std::uint64_t>& in_tension)
{
    for (const auto& c : ordered) {
        os << "About " << c.subject;
        if (!mode.no_charge && c.peak > 0.0f)
            os << "  [pressing ~" << std::lround(c.peak) << "/10]";
        os << ":\n";
        for (const auto* n : c.nodes) {
            os << "   - " << n->fact;
            if (!mode.no_charge) {
                os << "  (w" << std::lround(n->weight) << ")";
                if (in_tension.count(n->id)) os << "  [in tension]";
            }
            os << "\n";
        }
    }
}

void append_tension_crosslinks(std::ostringstream& os,
                               const WorldGraph& beliefs,
                               const std::unordered_set<std::uint64_t>& rendered_ids)
{
    bool header = false;
    std::unordered_set<std::string> seen;
    for (const auto& e : beliefs.all_edges()) {
        if (e.data.kind != "tension") continue;
        if (!rendered_ids.count(e.from_id) && !rendered_ids.count(e.to_id)) continue;
        const std::uint64_t a = std::min(e.from_id, e.to_id);
        const std::uint64_t b = std::max(e.from_id, e.to_id);
        if (!seen.insert(std::to_string(a) + "-" + std::to_string(b)).second) continue;
        const Node* na = beliefs.get_node(e.from_id);
        const Node* nb = beliefs.get_node(e.to_id);
        if (!na || !nb) continue;
        if (!header) { os << "Tensions (held, not resolved):\n"; header = true; }
        os << "   * \"" << short_fact(na->fact) << "\" <-> \""
           << short_fact(nb->fact) << "\"\n";
    }
}

}  // namespace

std::string CharacterMemory::render_thoughts(
    const std::vector<std::string>& subjects) const {

    std::vector<std::string> wanted;
    wanted.reserve(subjects.size());
    for (const auto& s : subjects)
        if (!s.empty()) wanted.push_back(str::to_lower(s));
    const bool all = wanted.empty();

    // Collect my live Thoughts (Active belief nodes), optionally scoped to the
    // requested subjects.  When scoped we keep the WHOLE chain for a matching
    // subject (root -> head), so a dispositional belief surfaces even if the
    // subject is not currently present.
    std::vector<const Node*> thoughts;
    beliefs_.for_each([&](const Node& n) {
        if (n.type != "belief" || n.state != NodeState::Active) return;
        if (all) { thoughts.push_back(&n); return; }
        for (const auto& e : n.entities) {
            const std::string el = str::to_lower(e);
            for (const auto& w : wanted)
                if (entity_matches(el, w)) { thoughts.push_back(&n); return; }
        }
    }, false);
    if (thoughts.empty()) return {};

    auto mode = ThoughtRenderMode::from_env();
    std::unordered_set<std::uint64_t> rendered;
    for (const auto* n : thoughts) rendered.insert(n->id);
    std::unordered_set<std::uint64_t> in_tension =
        mode.no_charge ? std::unordered_set<std::uint64_t>{}
                       : collect_tension_node_ids(beliefs_);

    auto ordered = group_and_order_thought_chains(thoughts, mode.no_charge);

    std::ostringstream os;
    append_thought_chains(os, ordered, mode, in_tension);
    if (!mode.no_charge)
        append_tension_crosslinks(os, beliefs_, rendered);

    return sanitize_utf8(os.str());
}

std::string CharacterMemory::view_of(const std::vector<std::string>& subjects) const {
    if (subjects.empty()) return {};
    return render_thoughts(subjects);
}

std::string CharacterMemory::pressing_thought(unsigned seed) const {
    float peak = 0.0f;
    std::vector<const Node*> charged;
    beliefs_.for_each([&](const Node& n) {
        if (n.type != "belief" || n.state != NodeState::Active) return;
        peak = std::max(peak, n.weight);
        charged.push_back(&n);
    }, false);
    if (charged.empty() || peak <= 0.0f) return {};

    // The most-charged Thoughts (within 1.0 of the peak); pick one seeded so the
    // choice is stable for a given (character, turn) but varies across turns.
    std::vector<const Node*> top;
    for (const auto* n : charged)
        if (n->weight >= peak - 1.0f) top.push_back(n);
    if (top.empty()) return {};
    std::mt19937 rng(seed);
    return sanitize_utf8(top[rng() % top.size()]->fact);
}

std::string CharacterMemory::charge_state() const {
    const auto in_tension = collect_tension_node_ids(beliefs_);

    const Node* top = nullptr;
    beliefs_.for_each([&](const Node& n) {
        if (n.type != "belief" || n.state != NodeState::Active) return;
        if (!top || n.weight > top->weight) top = &n;
    }, false);

    if (!top || top->weight < 8.0f) return {};  // only when something nears the ceiling
    return in_tension.count(top->id)
               ? std::string("a tension has become almost unbearable")
               : std::string("a conviction has hardened");
}

void CharacterMemory::route_fact(const std::string& fact,
                                 const std::vector<std::string>& entities,
                                 int turn) {
    if (fact.empty()) return;
    Node n;
    n.fact        = sanitize_utf8(fact);
    n.type        = "perception";
    n.state       = NodeState::Active;
    n.entities    = entities;
    n.created_at  = turn;
    n.valid_until = -1;
    beliefs_.add_node_chained(std::move(n), turn);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json CharacterMemory::to_json() const {
    nlohmann::json j;
    j["name"] = character_name_;
    j["belief_graph"] = beliefs_.to_json();
    j["core"] = nlohmann::json{
        {"text", core_.text},
        {"revised_at", core_.revised_at},
    };
    nlohmann::json streams = nlohmann::json::array();
    for (const auto& stream : streams_) {
        nlohmann::json row;
        row["id"] = stream.id;
        row["focus"] = stream.focus;
        row["status"] = stream.status;
        row["parent_id"] = stream.parent_id;
        row["closed_reason"] = stream.closed_reason;
        row["closed_summary"] = stream.closed_summary;
        nlohmann::json lines = nlohmann::json::array();
        for (const auto& line : stream.lines) {
            lines.push_back({{"turn", line.turn}, {"text", line.text}});
        }
        row["lines"] = std::move(lines);
        streams.push_back(std::move(row));
    }
    j["streams"] = std::move(streams);
    j["stream_seq"] = stream_seq_;
    return j;
}

CharacterMemory CharacterMemory::from_json(const nlohmann::json& j) {
    CharacterMemory cm(j.value("name", ""));

    if (j.contains("belief_graph"))
        cm.beliefs_ = WorldGraph::from_json(j["belief_graph"]);

    if (j.contains("core") && j["core"].is_object()) {
        cm.core_.text = j["core"].value("text", "");
        cm.core_.revised_at = j["core"].value("revised_at", -1);
    }

    cm.streams_.clear();
    if (j.contains("streams") && j["streams"].is_array()) {
        for (const auto& row : j["streams"]) {
            if (!row.is_object()) continue;
            MonologueStream stream;
            stream.id = row.value("id", "");
            stream.focus = row.value("focus", "");
            stream.status = row.value("status", "active");
            stream.parent_id = row.value("parent_id", "");
            stream.closed_reason = row.value("closed_reason", "");
            stream.closed_summary = row.value("closed_summary", "");
            if (row.contains("lines") && row["lines"].is_array()) {
                for (const auto& line : row["lines"]) {
                    if (!line.is_object()) continue;
                    stream.lines.push_back({
                        line.value("turn", 0),
                        line.value("text", ""),
                    });
                }
            }
            if (!stream.id.empty())
                cm.streams_.push_back(std::move(stream));
        }
    }
    cm.stream_seq_ = j.value("stream_seq", 0);
    cm.ensure_bootstrap("");
    return cm;
}

}  // namespace rhapsode
