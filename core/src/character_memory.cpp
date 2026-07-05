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

void CharacterMemory::set_reflection_llm_callback(ReflectionLLMCallback cb) { reflection_llm_cb_ = std::move(cb); }

// ---------------------------------------------------------------------------
// Subjective belief graph (beliefs_)
// ---------------------------------------------------------------------------

namespace {

std::string lower_str(std::string s) {
    return str::to_lower(s);
}

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
        std::string label = lower_str(s.substr(0, colon));
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

std::string CharacterMemory::render_thoughts(
    const std::vector<std::string>& subjects) const {

    std::vector<std::string> wanted;
    wanted.reserve(subjects.size());
    for (const auto& s : subjects)
        if (!s.empty()) wanted.push_back(lower_str(s));
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
            const std::string el = lower_str(e);
            for (const auto& w : wanted)
                if (entity_matches(el, w)) { thoughts.push_back(&n); return; }
        }
    }, false);
    if (thoughts.empty()) return {};

    // A/B baseline arm: when set, render a flat, recency-ordered view that
    // ignores weight and tension -- the control the charged interior is measured
    // against (see the design's Phase 3 experiments / ab-eval).
    const bool no_charge = std::getenv("RHAPSODE_BASELINE_NO_CHARGE") != nullptr;

    std::unordered_set<std::uint64_t> rendered;
    for (const auto* n : thoughts) rendered.insert(n->id);

    // Tension adjacency from typed edges -- the contradictions kept live.
    std::unordered_set<std::uint64_t> in_tension;
    if (!no_charge) {
        for (const auto& e : beliefs_.all_edges()) {
            if (e.data.kind != "tension") continue;
            in_tension.insert(e.from_id);
            in_tension.insert(e.to_id);
        }
    }

    // Group Thoughts into chains by subject (first entity, or "(myself)").
    std::unordered_map<std::string, std::vector<const Node*>> chains;
    for (const auto* n : thoughts) {
        std::string key = n->entities.empty() ? std::string("(myself)")
                                              : n->entities.front();
        chains[key].push_back(n);
    }

    struct Chain { std::string subject; std::vector<const Node*> nodes; float peak; };
    std::vector<Chain> ordered;
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
                  [](const Chain& a, const Chain& b) {
                      return a.nodes.back()->created_at > b.nodes.back()->created_at;
                  });
    else
        // Most-pressing chains lead.
        std::sort(ordered.begin(), ordered.end(),
                  [](const Chain& a, const Chain& b) { return a.peak > b.peak; });

    auto short_fact = [](const std::string& f) -> std::string {
        if (f.size() <= 80) return f;
        return truncate_utf8(f, 77) + "...";
    };

    std::ostringstream os;
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

    if (no_charge) return sanitize_utf8(os.str());

    // Explicit cross-links so the narrator sees exactly which Thoughts collide.
    bool header = false;
    std::unordered_set<std::string> seen;
    for (const auto& e : beliefs_.all_edges()) {
        if (e.data.kind != "tension") continue;
        if (!rendered.count(e.from_id) && !rendered.count(e.to_id)) continue;
        const std::uint64_t a = std::min(e.from_id, e.to_id);
        const std::uint64_t b = std::max(e.from_id, e.to_id);
        if (!seen.insert(std::to_string(a) + "-" + std::to_string(b)).second) continue;
        const Node* na = beliefs_.get_node(e.from_id);
        const Node* nb = beliefs_.get_node(e.to_id);
        if (!na || !nb) continue;
        if (!header) { os << "Tensions (held, not resolved):\n"; header = true; }
        os << "   * \"" << short_fact(na->fact) << "\" <-> \""
           << short_fact(nb->fact) << "\"\n";
    }

    return sanitize_utf8(os.str());
}

std::string CharacterMemory::build_prompt__interior_thoughts(
    const std::vector<std::string>& subjects, int turn) const
{
    std::string block;
    const std::string thoughts = render_thoughts(subjects);
    if (!thoughts.empty()) {
        block += "  Interior (live thoughts; weight = how much it presses, "
                  "tension = a contradiction held unresolved):\n";
        block += thoughts;
    }

    if (std::getenv("RHAPSODE_EXP_SURFACING")) {
        const unsigned seed =
            static_cast<unsigned>(std::hash<std::string>{}(char_name_)) ^
            static_cast<unsigned>(turn);
        if (std::string p = pressing_thought(seed); !p.empty())
            block += "  Pressing today: " + p + "\n";
    }
    if (std::getenv("RHAPSODE_EXP_CRISIS")) {
        if (std::string c = charge_state(); !c.empty())
            block += "  Charge: " + c + "\n";
    }
    return block;
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
    std::unordered_set<std::uint64_t> in_tension;
    for (const auto& e : beliefs_.all_edges())
        if (e.data.kind == "tension") {
            in_tension.insert(e.from_id);
            in_tension.insert(e.to_id);
        }

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

void CharacterMemory::reflect_perceptions(int turn) {
    if (!reflection_llm_cb_) {
        log() << "  [char_mem:" << char_name_
              << "] reflect skip: no reflection LLM callback\n" << std::flush;
        return;
    }

    // Gather unreflected perceptions (live "perception" nodes), grouped by subject.
    struct Perc { std::uint64_t id; std::string fact; std::vector<std::string> entities; };
    std::vector<Perc> perceptions;
    beliefs_.for_each([&](const Node& n) {
        if (n.type == "perception" && n.state == NodeState::Active)
            perceptions.push_back({n.id, n.fact, n.entities});
    }, false);
    if (perceptions.empty()) {
        log() << "  [char_mem:" << char_name_
              << "] reflect skip: no new perceptions\n" << std::flush;
        return;
    }

    // Subject -> indices of the perceptions about it.  Entity-less perceptions
    // fold under a synthetic "(world)" subject so general news still becomes
    // belief.  Indices (not just facts) so the belief can link to its evidence.
    std::unordered_map<std::string, std::vector<std::size_t>> by_subject;
    for (std::size_t i = 0; i < perceptions.size(); ++i) {
        const auto& p = perceptions[i];
        if (p.entities.empty()) {
            by_subject["(world)"].push_back(i);
        } else {
            for (const auto& e : p.entities) by_subject[e].push_back(i);
        }
    }

    // -- Reinforce-vs-decay tuning (the design's central knob). --
    constexpr int   kTouchHops  = 3;      // neighborhood a new Thought reinforces
    constexpr float kWeightCap  = 10.0f;  // ceiling on a Thought's pressure
    constexpr float kReinforce  = 1.0f;   // added to each touched neighbor
    constexpr float kDecay      = 0.9f;   // multiplied into untouched Thoughts
    constexpr float kCullFloor  = 0.05f;  // below this a Thought leaves the live pool

    // -- Batched reflection: ONE LLM call per character. --
    // Build a bucket per subject (its live priors + the perceptions just routed
    // about it), hand the whole set to the model in a single prompt, and read
    // back one thought per subject carrying its felt-pressure weight and its
    // relation to priors.  This collapses the old per-subject fan-out of 2-3
    // round-trips (form-thought + score_importance + classify_relation) into a
    // single call -- the dominant cost in the reflection pass.
    struct Bucket {
        std::string subject;
        std::vector<std::uint64_t> prior_ids;   // live beliefs about this subject
        std::string prior_text;
        std::vector<std::size_t> idxs;          // perceptions about this subject
    };
    std::vector<Bucket> buckets;
    buckets.reserve(by_subject.size());
    for (const auto& [subject, idxs] : by_subject) {
        Bucket b;
        b.subject = subject;
        b.idxs    = idxs;
        // My prior Thoughts about this subject (live "belief" nodes).  We NEVER
        // supersede them: contradictory Thoughts coexist and are kept as tension.
        const std::string subj_lower = lower_str(subject);
        beliefs_.for_each([&](const Node& n) {
            if (n.type != "belief" || n.state != NodeState::Active) return;
            for (const auto& e : n.entities) {
                if (entity_matches(lower_str(e), subj_lower)) {
                    b.prior_ids.push_back(n.id);
                    b.prior_text += "- " + n.fact + "\n";
                    return;
                }
            }
        }, false);
        buckets.push_back(std::move(b));
    }

    // One prompt enumerating every subject; the model returns a JSON object with
    // one entry per subject id.  Forming the Thought, rating its pressure, and
    // classifying its relation to priors all happen here at once.
    std::string prompt =
        "You are " + char_name_ + ".\n"
        + persona_line() +
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
        for (auto i : buckets[bi].idxs)
            prompt += "  - " + perceptions[i].fact + "\n";
    }
    prompt +=
        "\nRespond with ONLY a JSON object in exactly this shape, with one entry "
        "per subject id above:\n"
        "{\"thoughts\":[{\"id\":0,\"thought\":\"...\",\"weight\":7,"
        "\"relation\":\"tension\"}]}\n"
        "Use only straight ASCII double quotes.";

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
        const nlohmann::json parsed = try_parse_json(raw);
        const auto it = parsed.find("thoughts");
        if (it != parsed.end() && it->is_array()) {
            for (const auto& t : *it) {
                if (!t.is_object()) continue;
                const std::size_t bi = static_cast<std::size_t>(
                    json_number<int>(t, "id", -1));
                if (bi >= buckets.size()) continue;

                std::string belief =
                    strip_echoed_label(sanitize_utf8(t.value("thought", "")));
                if (belief.empty()) continue;

                const int w = std::clamp(json_number<int>(t, "weight", 5), 1, 10);
                const std::string rel = lower_str(t.value("relation", "evidence"));
                const std::string kind =
                    (rel.find("tension") != std::string::npos ||
                     rel.find("contradict") != std::string::npos)
                        ? "tension" : "evidence";

                const Bucket& bk = buckets[bi];
                Node nb;
                nb.fact        = sanitize_utf8(belief);
                nb.type        = "belief";
                nb.state       = NodeState::Active;
                nb.entities    = (bk.subject == "(world)")
                                     ? std::vector<std::string>{}
                                     : std::vector<std::string>{bk.subject};
                nb.created_at  = turn;
                nb.valid_until = -1;
                nb.weight      = static_cast<float>(w);
                Node& ref = beliefs_.add_node(std::move(nb));
                const std::uint64_t new_id = ref.id;
                new_thought_ids.push_back(new_id);

                // Relate to priors WITHOUT collapsing: tension (contradict) or
                // evidence (extend).  Both Thoughts stay live.
                for (auto pid : bk.prior_ids)
                    beliefs_.add_relation(new_id, pid, 1.0f, turn, kind);
                // Evidence: link the Thought to the perceptions it came from.
                for (auto i : bk.idxs)
                    beliefs_.add_relation(new_id, perceptions[i].id, 1.0f, turn,
                                          "evidence");
            }
        }
    }

    // Mark consolidated perceptions as no longer live (kept as history, not
    // re-reflected next turn).
    for (const auto& p : perceptions)
        beliefs_.set_valid_until(p.id, turn);

    // -- Weight lives by reinforce-vs-decay (the only writer of Thought weight).
    //    Each new Thought touches its local neighborhood (a few hops) and
    //    reinforces it; Thoughts left untouched this pass decay.  A foundational
    //    Thought stays charged while related material keeps touching it, and an
    //    external contradiction that links into a withheld Thought's neighborhood
    //    raises its pressure for free. --
    std::unordered_set<std::uint64_t> touched;
    for (auto nid : new_thought_ids) {
        touched.insert(nid);
        for (auto nb_id : beliefs_.neighbors_within(nid, kTouchHops)) {
            touched.insert(nb_id);
            Node* n = beliefs_.get_node(nb_id);
            if (n && n->type == "belief")
                n->weight = std::min(kWeightCap, n->weight + kReinforce);
        }
    }
    std::vector<std::uint64_t> live_thoughts;
    beliefs_.for_each([&](const Node& n) {
        if (n.type == "belief" && n.state == NodeState::Active)
            live_thoughts.push_back(n.id);
    }, false);
    int culled = 0;
    for (auto bid : live_thoughts) {
        if (touched.count(bid)) continue;
        Node* n = beliefs_.get_node(bid);
        if (!n) continue;
        n->weight *= kDecay;
        // Decay alone never pruned: a faded Thought stayed Active forever,
        // inflating the prior-gather scan and the rendered interior over a long
        // scene.  Once pressure decays below the floor, retire it from the live
        // pool (set_valid_until -- the same mechanism that consumes perceptions).
        if (n->weight < kCullFloor) {
            beliefs_.set_valid_until(bid, turn);
            ++culled;
            log() << "  [char_mem:" << char_name_ << "] culled #" << bid
                  << " (w=" << n->weight << "): " << n->fact << "\n";
        }
    }

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
