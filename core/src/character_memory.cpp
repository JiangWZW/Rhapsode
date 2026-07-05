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
// Construction + callback setters
// ---------------------------------------------------------------------------

CharacterMemory::CharacterMemory(std::string name)
    : char_name_(std::move(name)) {}

void CharacterMemory::set_reflection_llm_callback(LLMCallback cb) { reflection_llm_cb_ = std::move(cb); }

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

// Strip a label the model parroted back from the prompt ("My belief:",
// "Belief:", "My updated belief:", "My inner state:") so it never lands in the
// stored fact.  Only strips a short leading "<label>:" that mentions belief/state.
std::string strip_echoed_label(std::string s) {
    auto a = s.find_first_not_of(" \t\n\r\"");
    if (a == std::string::npos) return {};
    s = s.substr(a);
    auto colon = s.find(':');
    if (colon != std::string::npos && colon <= 24) {
        std::string label = str::to_lower(s.substr(0, colon));
        if (label.find("belief") != std::string::npos ||
            label.find("inner state") != std::string::npos) {
            auto rest = s.find_first_not_of(" \t\"", colon + 1);
            return rest == std::string::npos ? std::string() : s.substr(rest);
        }
    }
    return s;
}

}  // namespace

std::uint64_t CharacterMemory::seed_belief(const std::string& fact,
                                           const std::vector<std::string>& entities,
                                           int created_at,
                                           float weight) {
    if (fact.empty()) return 0;
    Node n;
    n.fact       = sanitize_utf8(fact);
    n.type       = "belief";
    n.state      = NodeState::Active;
    n.entities   = entities;
    n.created_at = created_at;
    n.valid_until = -1;
    n.weight     = weight;
    return beliefs_.add_node_chained(std::move(n), created_at).id;
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

namespace {

// -- reflect_perceptions helpers ----------------------------------------------
// One batched LLM call per character forms a Thought per subject, rates its
// felt-pressure, and classifies its relation to priors.  These helpers keep
// perceptions and buckets aligned by index through prompt -> parse -> apply.

struct Perc {
    std::uint64_t id;
    std::string fact;
    std::vector<std::string> entities;
};

struct ReflectionBucket {
    std::string subject;
    std::vector<std::uint64_t> prior_ids;      // live beliefs about this subject
    std::string prior_text;
    std::vector<std::size_t> perception_idxs;  // perceptions routed to this subject
};

struct ParsedThought {
    std::size_t bucket_idx;
    std::string belief;
    int weight;
    std::string kind;  // "tension" or "evidence"
};

struct WeightTuning {
    int   touch_hops = 3;
    float weight_cap = 10.0f;
    float reinforce  = 1.0f;
    float decay      = 0.9f;
    float cull_floor = 0.05f;
};

std::vector<Perc> gather_unreflected_perceptions(const WorldGraph& beliefs) {
    std::vector<Perc> perceptions;
    beliefs.for_each([&](const Node& n) {
        if (n.type == "perception" && n.state == NodeState::Active)
            perceptions.push_back({n.id, n.fact, n.entities});
    }, false);
    return perceptions;
}

// Entity-less perceptions fold under a synthetic "(world)" subject so general
// news still becomes belief.
std::unordered_map<std::string, std::vector<std::size_t>>
group_perceptions_by_subject(const std::vector<Perc>& perceptions) {
    std::unordered_map<std::string, std::vector<std::size_t>> by_subject;
    for (std::size_t i = 0; i < perceptions.size(); ++i) {
        const auto& p = perceptions[i];
        if (p.entities.empty()) {
            by_subject["(world)"].push_back(i);
        } else {
            for (const auto& e : p.entities) by_subject[e].push_back(i);
        }
    }
    return by_subject;
}

// Prior Thoughts about each subject are NEVER superseded: contradictory
// Thoughts coexist and are kept as tension.
std::vector<ReflectionBucket> build_reflection_buckets(
    const WorldGraph& beliefs,
    const std::vector<Perc>& perceptions,
    const std::unordered_map<std::string, std::vector<std::size_t>>& by_subject)
{
    std::vector<ReflectionBucket> buckets;
    buckets.reserve(by_subject.size());
    for (const auto& [subject, idxs] : by_subject) {
        ReflectionBucket b;
        b.subject           = subject;
        b.perception_idxs   = idxs;
        const std::string subj_lower = str::to_lower(subject);
        beliefs.for_each([&](const Node& n) {
            if (n.type != "belief" || n.state != NodeState::Active) return;
            for (const auto& e : n.entities) {
                if (entity_matches(str::to_lower(e), subj_lower)) {
                    b.prior_ids.push_back(n.id);
                    b.prior_text += "- " + n.fact + "\n";
                    return;
                }
            }
        }, false);
        buckets.push_back(std::move(b));
    }
    return buckets;
}

std::string build_reflection_prompt(
    const std::string& char_name,
    const std::string& description,
    const std::vector<ReflectionBucket>& buckets,
    const std::vector<Perc>& perceptions)
{
    std::string prompt =
        "You are " + char_name + ".\n"
        + (description.empty() ? std::string() : "Who I am: " + description + "\n") +
        "I have just perceived new things about several subjects. For EACH "
        "subject below, in ONE terse first-person sentence -- how I would "
        "actually think it, not prose -- say what this now makes me think or "
        "feel about it. Do not tidy it: if it sits uneasily with what I already "
        "think, let it. Rate how much it presses on me from 1 (settled or "
        "trivial) to 10 (unbearably charged). Say whether the new thought "
        "CONTRADICTS what I already thought (\"tension\") or EXTENDS / supports "
        "it (\"evidence\").\n\nSubjects:\n";
    for (std::size_t bi = 0; bi < buckets.size(); ++bi) {
        prompt += "[" + std::to_string(bi) + "] " + buckets[bi].subject + "\n";
        if (buckets[bi].prior_text.empty())
            prompt += "  What I already think: (nothing in particular yet)\n";
        else
            prompt += "  What I already think:\n" + buckets[bi].prior_text;
        prompt += "  What I just perceived:\n";
        for (auto i : buckets[bi].perception_idxs)
            prompt += "  - " + perceptions[i].fact + "\n";
    }
    prompt +=
        "\nRespond with ONLY a JSON object in exactly this shape, with one entry "
        "per subject id above:\n"
        "{\"thoughts\":[{\"id\":0,\"thought\":\"...\",\"weight\":7,"
        "\"relation\":\"tension\"}]}\n"
        "Use only straight ASCII double quotes.";
    return prompt;
}

std::vector<ParsedThought> parse_reflection_response(
    const std::string& raw, std::size_t bucket_count)
{
    std::vector<ParsedThought> thoughts;
    const nlohmann::json parsed = try_parse_json(raw);
    const auto it = parsed.find("thoughts");
    if (it == parsed.end() || !it->is_array())
        return thoughts;
    for (const auto& t : *it) {
        if (!t.is_object()) continue;
        const std::size_t bi = static_cast<std::size_t>(
            json_number<int>(t, "id", -1));
        if (bi >= bucket_count) continue;
        std::string belief =
            strip_echoed_label(sanitize_utf8(t.value("thought", "")));
        if (belief.empty()) continue;
        const int w = std::clamp(json_number<int>(t, "weight", 5), 1, 10);
        const std::string rel = str::to_lower(t.value("relation", "evidence"));
        const std::string kind =
            (rel.find("tension") != std::string::npos ||
             rel.find("contradict") != std::string::npos)
                ? "tension" : "evidence";
        thoughts.push_back({bi, std::move(belief), w, kind});
    }
    return thoughts;
}

// Creates belief nodes + tension/evidence edges to priors and perceptions.
std::vector<std::uint64_t> apply_reflection_thoughts(
    WorldGraph& beliefs,
    int turn,
    const std::vector<ReflectionBucket>& buckets,
    const std::vector<Perc>& perceptions,
    const std::vector<ParsedThought>& thoughts)
{
    std::vector<std::uint64_t> new_ids;
    for (const auto& th : thoughts) {
        const ReflectionBucket& bk = buckets[th.bucket_idx];
        Node nb;
        nb.fact        = sanitize_utf8(th.belief);
        nb.type        = "belief";
        nb.state       = NodeState::Active;
        nb.entities    = (bk.subject == "(world)")
                             ? std::vector<std::string>{}
                             : std::vector<std::string>{bk.subject};
        nb.created_at  = turn;
        nb.valid_until = -1;
        nb.weight      = static_cast<float>(th.weight);
        Node& ref = beliefs.add_node(std::move(nb));
        const std::uint64_t new_id = ref.id;
        new_ids.push_back(new_id);

        for (auto pid : bk.prior_ids)
            beliefs.add_relation(new_id, pid, 1.0f, turn, th.kind);
        for (auto i : bk.perception_idxs)
            beliefs.add_relation(new_id, perceptions[i].id, 1.0f, turn, "evidence");
    }
    return new_ids;
}

// Mark consolidated perceptions as no longer live (kept as history, not
// re-reflected next turn).
void consolidate_perceptions(WorldGraph& beliefs,
                             const std::vector<Perc>& perceptions,
                             int turn)
{
    for (const auto& p : perceptions)
        beliefs.set_valid_until(p.id, turn);
}

// Weight lives by reinforce-vs-decay (the only writer of Thought weight).
// Each new Thought touches its local neighborhood and reinforces it; Thoughts
// left untouched this pass decay.  Below the cull floor a Thought is retired
// from the live pool.  Returns the cull count.
int apply_reinforce_decay(WorldGraph& beliefs, int turn,
                          const std::vector<std::uint64_t>& new_thought_ids,
                          const WeightTuning& tuning,
                          const std::string& char_name)
{
    std::unordered_set<std::uint64_t> touched;
    for (auto nid : new_thought_ids) {
        touched.insert(nid);
        for (auto nb_id : beliefs.neighbors_within(nid, tuning.touch_hops)) {
            touched.insert(nb_id);
            Node* n = beliefs.get_node(nb_id);
            if (n && n->type == "belief")
                n->weight = std::min(tuning.weight_cap, n->weight + tuning.reinforce);
        }
    }
    std::vector<std::uint64_t> live_thoughts;
    beliefs.for_each([&](const Node& n) {
        if (n.type == "belief" && n.state == NodeState::Active)
            live_thoughts.push_back(n.id);
    }, false);
    int culled = 0;
    for (auto bid : live_thoughts) {
        if (touched.count(bid)) continue;
        Node* n = beliefs.get_node(bid);
        if (!n) continue;
        n->weight *= tuning.decay;
        if (n->weight < tuning.cull_floor) {
            beliefs.set_valid_until(bid, turn);
            ++culled;
            log() << "  [char_mem:" << char_name << "] culled #" << bid
                  << " (w=" << n->weight << "): " << n->fact << "\n";
        }
    }
    return culled;
}

}  // namespace

void CharacterMemory::reflect_perceptions(int turn, const std::string& description) {
    if (!reflection_llm_cb_) {
        log() << "  [char_mem:" << char_name_
              << "] reflect skip: no reflection LLM callback\n" << std::flush;
        return;
    }

    auto perceptions = gather_unreflected_perceptions(beliefs_);
    if (perceptions.empty()) {
        log() << "  [char_mem:" << char_name_
              << "] reflect skip: no new perceptions\n" << std::flush;
        return;
    }

    auto by_subject = group_perceptions_by_subject(perceptions);
    auto buckets    = build_reflection_buckets(beliefs_, perceptions, by_subject);
    auto prompt     = build_reflection_prompt(
        char_name_, description, buckets, perceptions);

    std::string raw;
    try {
        raw = reflection_llm_cb_(prompt);
    } catch (const std::exception& ex) {
        log() << "  [char_mem:" << char_name_
              << "] reflect_perceptions failed: " << ex.what() << "\n";
        raw.clear();
    }

    std::vector<std::uint64_t> new_thought_ids;
    if (!raw.empty()) {
        auto thoughts = parse_reflection_response(raw, buckets.size());
        new_thought_ids = apply_reflection_thoughts(
            beliefs_, turn, buckets, perceptions, thoughts);
    }

    consolidate_perceptions(beliefs_, perceptions, turn);
    int culled = apply_reinforce_decay(
        beliefs_, turn, new_thought_ids, WeightTuning{}, char_name_);

    log() << "  [char_mem:" << char_name_ << "] reflected "
          << perceptions.size() << " perception(s) into "
          << new_thought_ids.size() << " thought(s), culled "
          << culled << " faded\n" << std::flush;
}


// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json CharacterMemory::to_json() const {
    nlohmann::json j;
    j["name"]         = char_name_;
    j["belief_graph"] = beliefs_.to_json();
    return j;
}

CharacterMemory CharacterMemory::from_json(const nlohmann::json& j) {
    CharacterMemory cm(j.value("name", ""));

    // The subjective belief graph is the whole persisted mind.  Legacy saves
    // may still carry a "memories"/"edges" stream plus "self_state"/"next_id"/
    // "reflection_countdown" from the removed retrieval subsystem; those keys
    // are simply ignored on load (old saves open without crashing).
    if (j.contains("belief_graph"))
        cm.beliefs_ = WorldGraph::from_json(j["belief_graph"]);

    return cm;
}

}  // namespace rhapsode
