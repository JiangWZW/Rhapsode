#include "rhapsode/character_memory.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace rhapsode {

CharacterMemory::CharacterMemory(std::string name)
    : character_name_(std::move(name)) {
    ensure_bootstrap("");
}

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
                           bool no_charge,
                           const std::unordered_set<std::uint64_t>& in_tension)
{
    for (const auto& c : ordered) {
        os << "About " << c.subject;
        if (!no_charge && c.peak > 0.0f)
            os << "  [pressing ~" << std::lround(c.peak) << "/10]";
        os << ":\n";
        for (const auto* n : c.nodes) {
            os << "   - " << n->fact;
            if (!no_charge) {
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
            if (el.empty()) continue;
            for (const auto& w : wanted)
                if (el == w) { thoughts.push_back(&n); return; }
        }
    }, false);
    if (thoughts.empty()) return {};

    const bool no_charge =
        std::getenv("RHAPSODE_BASELINE_NO_CHARGE") != nullptr;
    std::unordered_set<std::uint64_t> rendered;
    for (const auto* n : thoughts) rendered.insert(n->id);
    std::unordered_set<std::uint64_t> in_tension =
        no_charge ? std::unordered_set<std::uint64_t>{}
                  : collect_tension_node_ids(beliefs_);

    auto ordered = group_and_order_thought_chains(thoughts, no_charge);

    std::ostringstream os;
    append_thought_chains(os, ordered, no_charge, in_tension);
    if (!no_charge)
        append_tension_crosslinks(os, beliefs_, rendered);

    return sanitize_utf8(os.str());
}

void CharacterMemory::append_objective(int turn, std::string kind,
                                       std::string text) {
    text = str::trim(sanitize_utf8(std::move(text)));
    if (text.empty()) return;
    if (kind != "take" && kind != "seen") kind = "seen";
    log() << "  [journal:" << character_name_ << "] " << kind
          << " t=" << turn << " " << text << "\n" << std::flush;
    objective_journal_.push_back({turn, std::move(kind), std::move(text)});
}

int CharacterMemory::claim_observation(int num_lines) {
    if (observation_pending()) return -1;
    const int i = observation_write_;
    observation_num_lines_[i] = num_lines;
    observation_pending_[i] = true;
    observation_write_ = (observation_write_ + 1) % kStagingBuffers;
    return i;
}

void CharacterMemory::release_observation(int i) {
    observation_pending_[i] = false;
}

bool CharacterMemory::observation_pending() const {
    for (int i = 0; i < kStagingBuffers; ++i)
        if (observation_pending_[i]) return true;
    return false;
}

int CharacterMemory::claim_monologue(int num_lines, MonologueBuildContext ctx) {
    if (monologue_pending()) return -1;
    const int i = monologue_write_;
    monologue_num_lines_[i] = num_lines;
    monologue_pending_[i] = true;
    monologue_ctx_[i] = std::move(ctx);
    monologue_write_ = (monologue_write_ + 1) % kStagingBuffers;
    return i;
}

void CharacterMemory::release_monologue(int i) {
    monologue_pending_[i] = false;
}

bool CharacterMemory::monologue_pending() const {
    for (int i = 0; i < kStagingBuffers; ++i)
        if (monologue_pending_[i]) return true;
    return false;
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
            lines.push_back({
                {"turn", line.turn},
                {"text", line.text},
                {"seq", line.seq},
            });
        }
        row["lines"] = std::move(lines);
        streams.push_back(std::move(row));
    }
    j["streams"] = std::move(streams);
    nlohmann::json journal = nlohmann::json::array();
    for (const auto& line : objective_journal_) {
        journal.push_back({
            {"turn", line.turn},
            {"kind", line.type},
            {"text", line.text},
        });
    }
    j["monologue_consumed_lines"] = monologue_consumed_lines_;
    j["stream_cnt"] = monologue_stream_cnt_;

    j["objective_journal"] = std::move(journal);
    j["observation_consumed_lines"] = objective_journal_consumed_lines_;
    j["line_cnt"] = objective_journal_line_cnt_;
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
                        line.value("seq", 0),
                    });
                }
            }
            if (!stream.id.empty())
                cm.streams_.push_back(std::move(stream));
        }
    }
    cm.objective_journal_.clear();
    if (j.contains("objective_journal") && j["objective_journal"].is_array()) {
        for (const auto& line : j["objective_journal"]) {
            if (!line.is_object()) continue;
            ObjectiveLine row;
            row.turn = line.value("turn", 0);
            row.type = line.value("kind", "seen");
            row.text = line.value("text", "");
            if (!row.text.empty())
                cm.objective_journal_.push_back(std::move(row));
        }
    }

    cm.monologue_consumed_lines_ = j.value(
        "monologue_consumed_lines",
        j.value("mono_consumed_lines", j.value("mono_consumed_upto", 0)));
    cm.monologue_stream_cnt_ = j.value(
        "stream_cnt", j.value("next_stream_id", j.value("stream_seq", 0)));

    cm.objective_journal_consumed_lines_ = j.value(
        "observation_consumed_lines",
        j.value("obs_consumed_lines", j.value("obs_consumed_upto", 0)));
    if (j.contains("line_cnt") || j.contains("next_line_id") ||
        j.contains("line_seq")) {
        cm.objective_journal_line_cnt_ = j.value(
            "line_cnt", j.value("next_line_id", j.value("line_seq", 0)));
    } else {
        int seq = 0;
        for (auto& stream : cm.streams_) {
            for (auto& line : stream.lines)
                line.seq = ++seq;
        }
        cm.objective_journal_line_cnt_ = seq;
    }
    cm.ensure_bootstrap("");
    return cm;
}

void CharacterMemory::append_monologue_line(MonologueStream& stream, int turn,
                                  std::string text) {
    stream.lines.push_back({turn, std::move(text), ++objective_journal_line_cnt_});
}

}  // namespace rhapsode
