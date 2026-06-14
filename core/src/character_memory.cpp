#include "rhapsode/character_memory.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
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

void CharacterMemory::set_embed_callback(EmbedCallback cb)            { embed_cb_ = std::move(cb); }
void CharacterMemory::set_store_callback(StoreCallback cb)            { store_cb_ = std::move(cb); }
void CharacterMemory::set_query_callback(QueryCallback cb)            { query_cb_ = std::move(cb); }
void CharacterMemory::set_reflection_llm_callback(ReflectionLLMCallback cb) { reflection_llm_cb_ = std::move(cb); }

// ---------------------------------------------------------------------------
// Private graph helpers
// ---------------------------------------------------------------------------

CharacterMemory::MemoryNode& CharacterMemory::add(MemoryNode node) {
    if (node.id == 0)
        node.id = static_cast<std::uint64_t>(next_node_id_++);
    else if (static_cast<int>(node.id) >= next_node_id_)
        next_node_id_ = static_cast<int>(node.id) + 1;

    if (node.last_accessed == 0)
        node.last_accessed = static_cast<short>(node.created_at);

    auto v = boost::add_vertex(std::move(node), graph_);
    return graph_[v];
}

CharacterMemory::MemoryNode* CharacterMemory::get(std::uint64_t id) {
    auto [vi, vi_end] = boost::vertices(graph_);
    for (; vi != vi_end; ++vi) {
        if (graph_[*vi].id == id)
            return &graph_[*vi];
    }
    return nullptr;
}

bool CharacterMemory::add_edge(std::uint64_t from_id, std::uint64_t to_id,
                               EdgeData& edge_payload) {
    using Vertex = MemGraph::vertex_descriptor;
    Vertex from_v = 0, to_v = 0;
    bool found_from = false, found_to = false;

    auto [vi, vi_end] = boost::vertices(graph_);
    for (; vi != vi_end; ++vi) {
        if (graph_[*vi].id == from_id) { from_v = *vi; found_from = true; }
        if (graph_[*vi].id == to_id)   { to_v   = *vi; found_to   = true; }
        if (found_from && found_to) break;
    }
    if (!found_from || !found_to) return false;

    boost::add_edge(from_v, to_v, edge_payload, graph_);
    return true;
}

// ---------------------------------------------------------------------------
// score_importance  (LLM poignancy scoring, GA-style)
// ---------------------------------------------------------------------------

int CharacterMemory::score_importance(const std::string& description) const {
    if (!reflection_llm_cb_) return 5;

    std::string prompt =
        "You are " + char_name_ + ".\n"
        "On a scale of 1 to 10, how much does the following press on me right "
        "now -- how much it tears at me, drives me, or refuses to settle "
        "(1 = settled or trivial, 10 = unbearably charged):\n"
        "\"" + description + "\"\n"
        "Respond with only a number.";

    std::string response;
    try {
        response = reflection_llm_cb_(prompt);
    } catch (const std::exception& e) {
        std::cerr << "  [char_mem:" << char_name_
                  << "] score_importance LLM failed: " << e.what() << "\n";
        return 5;
    }

    int score = 5;
    try {
        score = std::stoi(response);
    } catch (...) {
        for (char c : response) {
            if (c >= '1' && c <= '9') { score = c - '0'; break; }
        }
    }
    return std::clamp(score, 1, 10);
}

// ---------------------------------------------------------------------------
// trim_llm_response  (shared post-processing for LLM condensation calls)
// ---------------------------------------------------------------------------

namespace {

std::string trim_llm_response(const std::string& response, size_t max_len) {
    auto s = response.find_first_not_of(" \t\n\r\"");
    if (s == std::string::npos) return {};
    auto e = response.find_last_not_of(" \t\n\r\"");
    auto trimmed = response.substr(s, e - s + 1);
    if (trimmed.size() > max_len) return {};
    return trimmed;
}

}  // namespace

// ---------------------------------------------------------------------------
// distill  (compress long text into concise memory via local LLM)
// ---------------------------------------------------------------------------

std::string CharacterMemory::distill(const std::string& text) const {
    if (text.size() <= 200 || !reflection_llm_cb_) return text;

    std::string prompt =
        "You are " + char_name_ + ".\n"
        + persona_line() +
        "Condense the following into one concise sentence in the first person, "
        "as I would remember it ('I ...'), keeping key facts and names:\n\""
        + text + "\"\nMemory:";

    try {
        auto trimmed = trim_llm_response(reflection_llm_cb_(prompt), text.size());
        return trimmed.empty() ? text : trimmed;
    } catch (const std::exception& ex) {
        std::cerr << "  [char_mem:" << char_name_
                  << "] distill failed: " << ex.what() << "\n";
        return text;
    }
}

// ---------------------------------------------------------------------------
// observe / speak  (record a memory, drain countdown, embed+store)
// ---------------------------------------------------------------------------

namespace {

std::string collection_for(const std::string& name) {
    std::string safe;
    safe.reserve(5 + name.size());
    safe += "char_";
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-')
            safe += c;
        else
            safe += '_';
    }
    return safe;
}

void embed_and_store(const std::string& collection,
                     const CharacterMemory::MemoryNode& node,
                     const EmbedCallback& embed_cb,
                     const StoreCallback& store_cb) {
    if (!embed_cb || !store_cb) return;

    auto emb_json = embed_cb(node.content);
    std::string doc_id = "mem_" + std::to_string(node.id);
    nlohmann::json meta = {
        {"type",       static_cast<int>(node.type)},
        {"created_at", node.created_at},
        {"weight",     node.weight},
        {"depth",      node.depth},
    };
    store_cb(collection, doc_id, node.content, emb_json, meta.dump());
}

}  // namespace

void CharacterMemory::observe(const std::string& scene_context, int turn) {
    MemoryNode node;
    node.type       = kEvent;
    node.content    = distill(scene_context);
    node.created_at = turn;
    node.weight     = static_cast<float>(score_importance(node.content));

    auto& stored = add(std::move(node));
    reflection_countdown_ -= static_cast<int64_t>(stored.weight);

    embed_and_store(collection_for(char_name_), stored, embed_cb_, store_cb_);

    std::cerr << "  [char_mem:" << char_name_ << "] observe turn=" << turn
              << " w=" << stored.weight
              << " countdown=" << reflection_countdown_ << "\n" << std::flush;
}

void CharacterMemory::speak(const std::string& scene_context, int turn) {
    MemoryNode node;
    node.type       = kChat;
    node.content    = distill(scene_context);
    node.created_at = turn;
    node.weight     = static_cast<float>(score_importance(node.content));

    auto& stored = add(std::move(node));
    reflection_countdown_ -= static_cast<int64_t>(stored.weight);

    embed_and_store(collection_for(char_name_), stored, embed_cb_, store_cb_);

    std::cerr << "  [char_mem:" << char_name_ << "] speak turn=" << turn
              << " w=" << stored.weight
              << " countdown=" << reflection_countdown_ << "\n" << std::flush;
}

void CharacterMemory::seed_from_graph(const std::string& fact, int created_at) {
    MemoryNode node;
    node.type       = kEvent;
    node.content    = fact;
    node.created_at = created_at;
    node.weight     = 0.0f;
    add(std::move(node));
}

// ---------------------------------------------------------------------------
// Subjective belief graph (beliefs_)
// ---------------------------------------------------------------------------

namespace {

std::string lower_str(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
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

void CharacterMemory::seed_belief(const std::string& fact,
                                  const std::vector<std::string>& entities,
                                  int created_at) {
    if (fact.empty()) return;
    Node n;
    n.fact       = sanitize_utf8(fact);
    n.type       = "belief";
    n.state      = NodeState::Active;
    n.entities   = entities;
    n.created_at = created_at;
    n.valid_until = -1;
    beliefs_.add_node_chained(std::move(n), created_at);
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
        return f.size() <= 80 ? f : f.substr(0, 77) + "...";
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

    if (no_charge) return os.str();

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

    return os.str();
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
    return top[rng() % top.size()]->fact;
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
        std::cerr << "  [char_mem:" << char_name_
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
        std::cerr << "  [char_mem:" << char_name_
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
        std::cerr << "  [char_mem:" << char_name_
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
            std::cerr << "  [char_mem:" << char_name_ << "] culled #" << bid
                      << " (w=" << n->weight << "): " << n->fact << "\n";
        }
    }

    std::cerr << "  [char_mem:" << char_name_ << "] reflected "
              << perceptions.size() << " perception(s) into "
              << new_thought_ids.size() << " thought(s), culled "
              << culled << " faded\n" << std::flush;
}

// ---------------------------------------------------------------------------
// retrieve  (three-signal scoring + 1-hop BFS)
// ---------------------------------------------------------------------------

std::string CharacterMemory::retrieve(const std::string& query, int top_k) {
    if (!embed_cb_ || !query_cb_) return "";

    int now_turn = 0;
    {
        auto [vi, vi_end] = boost::vertices(graph_);
        for (; vi != vi_end; ++vi)
            now_turn = std::max(now_turn, static_cast<int>(graph_[*vi].created_at));
    }

    auto query_emb = embed_cb_(query);
    std::string collection = collection_for(char_name_);
    nlohmann::json where = nlohmann::json::object();
    auto raw_str = query_cb_(collection, query_emb, top_k * 2, where.dump());
    auto raw = nlohmann::json::parse(raw_str, nullptr, false);
    if (raw.is_discarded()) return "";

    auto ids_arr = raw.value("ids", nlohmann::json::array());
    if (ids_arr.empty() || ids_arr[0].empty()) return "";

    auto& doc_ids   = ids_arr[0];
    auto& distances = raw["distances"][0];
    auto& metadatas = raw["metadatas"][0];
    auto& documents = raw["documents"][0];

    struct Candidate {
        std::uint64_t id;
        float score;
        std::string content;
        int type;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(doc_ids.size());

    constexpr float kDecay = 0.995f;

    for (size_t i = 0; i < doc_ids.size(); ++i) {
        std::string did = doc_ids[i].get<std::string>();
        // doc_id format: "mem_<id>"
        std::uint64_t mem_id = 0;
        if (did.size() > 4 && did.substr(0, 4) == "mem_") {
            try { mem_id = std::stoull(did.substr(4)); } catch (...) { continue; }
        } else {
            continue;
        }

        MemoryNode* node = get(mem_id);
        if (!node) continue;

        float dist = distances[i].get<float>();
        float relevance = std::clamp(1.0f - dist, 0.0f, 1.0f);

        int age = now_turn - static_cast<int>(node->last_accessed);
        float recency = std::pow(kDecay, static_cast<float>(std::max(age, 0)));

        float importance = node->weight / 10.0f;

        float combined = recency + relevance + importance;
        candidates.push_back({mem_id, combined, node->content,
                              static_cast<int>(node->type)});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

    if (static_cast<int>(candidates.size()) > top_k)
        candidates.resize(static_cast<size_t>(top_k));

    // 1-hop BFS: enrich with graph neighbors
    std::unordered_set<std::uint64_t> seen;
    for (const auto& c : candidates)
        seen.insert(c.id);

    std::vector<Candidate> extras;
    for (const auto& c : candidates) {
        // Find vertex for this candidate
        auto [vi, vi_end] = boost::vertices(graph_);
        for (; vi != vi_end; ++vi) {
            if (graph_[*vi].id != c.id) continue;

            // Out-edges (kThought -> evidence)
            auto [ei, ei_end] = boost::out_edges(*vi, graph_);
            for (; ei != ei_end; ++ei) {
                auto target = boost::target(*ei, graph_);
                auto& tnode = graph_[target];
                if (seen.insert(tnode.id).second)
                    extras.push_back({tnode.id, 0.0f, tnode.content,
                                      static_cast<int>(tnode.type)});
            }

            // In-edges (reflections pointing to this node)
            auto [all_vi, all_vi_end] = boost::vertices(graph_);
            for (; all_vi != all_vi_end; ++all_vi) {
                auto [oei, oei_end] = boost::out_edges(*all_vi, graph_);
                for (; oei != oei_end; ++oei) {
                    if (boost::target(*oei, graph_) == *vi) {
                        auto& snode = graph_[*all_vi];
                        if (seen.insert(snode.id).second)
                            extras.push_back({snode.id, 0.0f, snode.content,
                                              static_cast<int>(snode.type)});
                    }
                }
            }
            break;
        }
    }

    for (auto& ex : extras)
        candidates.push_back(std::move(ex));

    // Update last_accessed
    for (const auto& c : candidates) {
        MemoryNode* node = get(c.id);
        if (node) node->last_accessed = static_cast<short>(now_turn);
    }

    // Format output
    static const char* kTypeLabel[] = {"event", "thought", "chat", "?"};
    std::ostringstream os;
    for (const auto& c : candidates) {
        int t = std::clamp(c.type, 0, 3);
        os << "- [" << kTypeLabel[t] << "] " << c.content << "\n";
    }
    return os.str();
}

// ---------------------------------------------------------------------------
// briefing  (synthesize retrieved memories into a compact paragraph)
// ---------------------------------------------------------------------------

std::string CharacterMemory::briefing(const std::string& query, int top_k) {
    std::string raw = retrieve(query, top_k);
    if (raw.empty() || !reflection_llm_cb_) return raw;

    std::string prompt =
        "You are " + char_name_ + ".\n"
        + persona_line() +
        "Using these memories, write 3-5 sentences in the first person ('I ...') "
        "describing what I know and how I feel about the current situation:\n"
        + raw + "\nMy thoughts:";

    try {
        // A briefing is a *summary* of the retrieved memories, so it is bounded
        // by a fixed length -- NOT by raw.size() (the raw list is often shorter
        // than a 3-5 sentence first-person summary, which would wrongly discard
        // every valid completion and fall back to the bare list).
        constexpr size_t kBriefingMaxChars = 1200;
        auto trimmed = trim_llm_response(reflection_llm_cb_(prompt), kBriefingMaxChars);
        return trimmed.empty() ? raw : trimmed;
    } catch (const std::exception& ex) {
        std::cerr << "  [char_mem:" << char_name_
                  << "] briefing failed: " << ex.what() << "\n";
        return raw;
    }
}

// ---------------------------------------------------------------------------
// update_self_state  (fold previous inner state forward with recent events)
// ---------------------------------------------------------------------------

void CharacterMemory::update_self_state(int turn) {
    if (!reflection_llm_cb_) return;  // no LLM available -> keep prior state

    // Fold ONLY my own recent perceptions (what *I* witnessed) forward into my
    // inner state -- never the shared narration -- so the state stays subjective.
    // A character who was not present perceives nothing and keeps its prior state.
    std::vector<std::pair<int, std::string>> seen;
    beliefs_.for_each([&](const Node& n) {
        if (n.type == "perception")
            seen.emplace_back(n.created_at, n.fact);
    }, true);  // include consolidated perceptions -- still things I witnessed
    if (seen.empty()) return;

    std::sort(seen.begin(), seen.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    std::string recent_events;
    for (std::size_t i = 0; i < std::min<std::size_t>(seen.size(), 6); ++i)
        recent_events += "- " + seen[i].second + "\n";

    std::string prompt =
        "You are " + char_name_ + ".\n"
        + persona_line() +
        "A moment ago, my state of mind was:\n\"" +
        (self_state_.empty() ? std::string("(nothing in particular yet)") : self_state_) +
        "\"\n\nWhat I have lately perceived:\n" + recent_events +
        "\n\nWrite my updated inner state in the first person (3-5 sentences): "
        "what I now know, feel, and want. Keep carrying what still weighs on me; "
        "let go only of what is truly resolved.\nMy inner state:";

    try {
        auto trimmed = trim_llm_response(reflection_llm_cb_(prompt), prompt.size());
        if (!trimmed.empty())
            self_state_ = std::move(trimmed);
    } catch (const std::exception& ex) {
        std::cerr << "  [char_mem:" << char_name_
                  << "] update_self_state failed: " << ex.what() << "\n";
        return;
    }

    std::cerr << "  [char_mem:" << char_name_ << "] self_state updated turn=" << turn
              << " (" << self_state_.size() << " chars)\n" << std::flush;
}

// ---------------------------------------------------------------------------
// reflect  (GA reflection pipeline with dedup)
// ---------------------------------------------------------------------------

namespace {

std::string strip_md_bold(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '*' && i + 1 < s.size() && s[i + 1] == '*') {
            ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string strip_leading_markers(const std::string& line) {
    size_t i = 0;
    while (i < line.size() && (std::isdigit(static_cast<unsigned char>(line[i])) ||
           line[i] == '.' || line[i] == ')' || line[i] == ' ' ||
           line[i] == '-' || line[i] == '*' || line[i] == '#' ||
           line[i] == '>' || line[i] == '\t'))
        ++i;
    return line.substr(i);
}

std::string strip_insight_prefix(const std::string& s) {
    if (s.size() > 8 && (s.compare(0, 7, "Insight") == 0 || s.compare(0, 7, "insight") == 0)) {
        auto colon = s.find(':');
        if (colon != std::string::npos && colon < 30) {
            auto rest = s.find_first_not_of(" \t", colon + 1);
            if (rest != std::string::npos) return s.substr(rest);
        }
    }
    return s;
}

std::string trim_ws(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        auto s = strip_leading_markers(line);
        s = strip_md_bold(s);
        s = strip_insight_prefix(s);
        if (!s.empty() && s.back() == ':') s.pop_back();
        s = trim_ws(s);
        if (s.size() < 10) continue;
        lines.push_back(std::move(s));
    }
    return lines;
}

}  // namespace

bool CharacterMemory::needs_reflection() const {
    return reflection_countdown_ <= 0 && reflection_llm_cb_ != nullptr;
}

void CharacterMemory::reflect() {
    if (reflection_countdown_ > 0) return;
    if (!reflection_llm_cb_) return;

    std::cerr << "  [char_mem:" << char_name_
              << "] reflection triggered (countdown=" << reflection_countdown_ << ")\n"
              << std::flush;

    // Step 1: Gather recent memories sorted by created_at descending
    struct TimedNode { int64_t created_at; std::string content; int type; };
    std::vector<TimedNode> recent;
    {
        auto [vi, vi_end] = boost::vertices(graph_);
        for (; vi != vi_end; ++vi) {
            if (graph_[*vi].depth >= 3) continue;
            recent.push_back({graph_[*vi].created_at, graph_[*vi].content,
                              static_cast<int>(graph_[*vi].type)});
        }
    }
    std::sort(recent.begin(), recent.end(),
              [](const TimedNode& a, const TimedNode& b) { return a.created_at > b.created_at; });
    if (recent.size() > 30) recent.resize(30);

    if (recent.empty()) {
        reflection_countdown_ = kReflectionInterval;
        return;
    }

    std::ostringstream mem_list;
    for (size_t i = 0; i < recent.size(); ++i)
        mem_list << (i + 1) << ". " << recent[i].content << "\n";

    // Step 2: Generate focal questions
    std::string focal_prompt =
        "You are " + char_name_ + ".\n"
        + persona_line() +
        "Given only the statements below, what are the 3 most salient high-level "
        "questions I would ask myself about my recent experiences?\n" +
        mem_list.str() +
        "List exactly 3 questions, one per line.\n";

    std::vector<std::string> questions;
    try {
        auto response = reflection_llm_cb_(focal_prompt);
        questions = split_lines(response);
        if (questions.size() > 3) questions.resize(3);
    } catch (const std::exception& e) {
        std::cerr << "  [char_mem:" << char_name_
                  << "] focal question LLM failed: " << e.what() << "\n";
        reflection_countdown_ = kReflectionInterval;
        return;
    }

    if (questions.empty()) {
        reflection_countdown_ = kReflectionInterval;
        return;
    }

    int now_turn = 0;
    {
        auto [vi, vi_end] = boost::vertices(graph_);
        for (; vi != vi_end; ++vi)
            now_turn = std::max(now_turn, static_cast<int>(graph_[*vi].created_at));
    }

    std::string collection = collection_for(char_name_);
    int stored_count = 0;

    // Step 3: For each focal question, retrieve evidence and generate insights
    for (const auto& question : questions) {
        std::string evidence = retrieve(question, 10);
        if (evidence.empty()) continue;

        std::string insight_prompt =
            "You are " + char_name_ + ".\n"
            + persona_line() +
            "Statements about myself:\n" +
            evidence + "\n"
            "What 2 high-level insights can I infer from the above?\n"
            "(write each as a first-person statement, e.g. \"I ...\")\n";

        std::vector<std::string> insights;
        try {
            auto response = reflection_llm_cb_(insight_prompt);
            insights = split_lines(response);
            if (insights.size() > 2) insights.resize(2);
        } catch (const std::exception& e) {
            std::cerr << "  [char_mem:" << char_name_
                      << "] insight LLM failed: " << e.what() << "\n";
            continue;
        }

        // Collect evidence node IDs for edge linking
        // Parse the evidence text to find which nodes were used
        // (retrieve() returns formatted text; we need the IDs from the last retrieve call)
        // For simplicity, re-query to get the IDs directly
        std::vector<std::uint64_t> evidence_ids;
        short max_evidence_depth = 0;
        if (embed_cb_ && query_cb_) {
            auto q_emb = embed_cb_(question);
            nlohmann::json w = nlohmann::json::object();
            auto r_str = query_cb_(collection, q_emb, 10, w.dump());
            auto r = nlohmann::json::parse(r_str, nullptr, false);
            if (!r.is_discarded()) {
                auto r_ids = r.value("ids", nlohmann::json::array());
                if (!r_ids.empty() && !r_ids[0].empty()) {
                    for (const auto& did : r_ids[0]) {
                        std::string id_str = did.get<std::string>();
                        if (id_str.size() > 4 && id_str.substr(0, 4) == "mem_") {
                            try {
                                auto mid = std::stoull(id_str.substr(4));
                                MemoryNode* en = get(mid);
                                if (en && en->depth >= 3) continue;
                                evidence_ids.push_back(mid);
                                if (en) max_evidence_depth = std::max(max_evidence_depth, en->depth);
                            } catch (...) {}
                        }
                    }
                }
            }
        }

        for (const auto& insight : insights) {
            if (insight.empty()) continue;

            // Dedup: check if a very similar kThought already exists
            bool is_redundant = false;
            if (embed_cb_ && query_cb_) {
                auto ins_emb = embed_cb_(insight);
                nlohmann::json thought_filter = {{"type", static_cast<int>(kThought)}};
                auto dup_str = query_cb_(collection, ins_emb, 3, thought_filter.dump());
                auto dup = nlohmann::json::parse(dup_str, nullptr, false);
                if (!dup.is_discarded()) {
                    auto dup_ids = dup.value("ids", nlohmann::json::array());
                    if (!dup_ids.empty() && !dup_ids[0].empty()) {
                        auto& dup_dists = dup["distances"][0];
                        auto& dup_docs  = dup["documents"][0];
                        if (!dup_dists.empty()) {
                            float closest_dist = dup_dists[0].get<float>();
                            if (closest_dist < 0.15f) {
                                std::string existing = dup_docs[0].get<std::string>();
                                std::string dedup_prompt =
                                    "Existing thought of " + char_name_ + ": \"" + existing + "\"\n"
                                    "Proposed new thought: \"" + insight + "\"\n"
                                    "Does the new thought say essentially the same thing as the existing one?\n"
                                    "Answer JSON: {\"redundant\": true/false}\n";
                                try {
                                    auto dedup_resp = reflection_llm_cb_(dedup_prompt);
                                    if (dedup_resp.empty()) {
                                        is_redundant = true;
                                    } else {
                                        auto dj = try_parse_json(dedup_resp);
                                        is_redundant = dj.value("redundant", false);
                                    }
                                } catch (...) {
                                    is_redundant = true;
                                }

                                if (is_redundant) {
                                    std::cerr << "  [char_mem:" << char_name_
                                              << "] dedup: skipping redundant insight\n";
                                }
                            }
                        }
                    }
                }
            }
            if (is_redundant) continue;

            // Store the new insight as kThought
            MemoryNode thought;
            thought.type       = kThought;
            thought.content    = insight;
            thought.created_at = now_turn;
            thought.weight     = static_cast<float>(score_importance(insight));
            thought.depth      = max_evidence_depth + 1;

            auto& stored = add(std::move(thought));
            embed_and_store(collection, stored, embed_cb_, store_cb_);

            // Link insight -> evidence
            for (auto eid : evidence_ids) {
                EdgeData ed;
                ed.weight = 1.0f;
                ed.created_at = now_turn;
                ed.active = true;
                add_edge(stored.id, eid, ed);
            }

            ++stored_count;
            std::cerr << "  [char_mem:" << char_name_ << "] reflection stored insight id="
                      << stored.id << " depth=" << stored.depth << "\n" << std::flush;
        }
    }

    reflection_countdown_ = kReflectionInterval;
    std::cerr << "  [char_mem:" << char_name_ << "] reflection complete, "
              << stored_count << " insight(s) stored, countdown reset\n" << std::flush;
}

// ---------------------------------------------------------------------------
// sync_to_chroma  (re-embed all memories at session start)
// ---------------------------------------------------------------------------

void CharacterMemory::sync_to_chroma() {
    if (!embed_cb_ || !store_cb_) return;

    std::string collection = collection_for(char_name_);
    int count = 0;

    auto [vi, vi_end] = boost::vertices(graph_);
    for (; vi != vi_end; ++vi) {
        embed_and_store(collection, graph_[*vi], embed_cb_, store_cb_);
        ++count;
    }

    std::cerr << "  [char_mem:" << char_name_ << "] synced " << count
              << " memories to ChromaDB\n" << std::flush;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json CharacterMemory::to_json() const {
    nlohmann::json j;
    j["name"]                 = char_name_;
    j["next_id"]              = next_node_id_;
    j["reflection_countdown"] = reflection_countdown_;
    j["self_state"]           = self_state_;
    j["belief_graph"]         = beliefs_.to_json();

    auto& memories = j["memories"] = nlohmann::json::array();
    auto [vi, vi_end] = boost::vertices(graph_);
    for (; vi != vi_end; ++vi) {
        const auto& n = graph_[*vi];
        memories.push_back({
            {"id",            n.id},
            {"type",          static_cast<int>(n.type)},
            {"content",       n.content},
            {"created_at",    n.created_at},
            {"weight",        n.weight},
            {"depth",         n.depth},
            {"last_accessed", n.last_accessed},
        });
    }

    auto& edges = j["edges"] = nlohmann::json::array();
    auto [ei, ei_end] = boost::edges(graph_);
    for (; ei != ei_end; ++ei) {
        auto src = boost::source(*ei, graph_);
        auto tgt = boost::target(*ei, graph_);
        edges.push_back({
            {"from",       graph_[src].id},
            {"to",         graph_[tgt].id},
            {"weight",     graph_[*ei].weight},
            {"created_at", graph_[*ei].created_at},
        });
    }

    return j;
}

CharacterMemory CharacterMemory::from_json(const nlohmann::json& j) {
    CharacterMemory cm(j.value("name", ""));
    cm.next_node_id_ = j.value("next_id", 1);
    cm.self_state_   = j.value("self_state", "");

    // Subjective belief graph.  Absent in pre-refactor saves -> empty graph
    // (migration: old saves load without crashing; beliefs re-accrue in play).
    if (j.contains("belief_graph"))
        cm.beliefs_ = WorldGraph::from_json(j["belief_graph"]);

    // Backward compat: old format uses importance_trigger_curr
    if (j.contains("reflection_countdown"))
        cm.reflection_countdown_ = j["reflection_countdown"].get<int64_t>();
    else if (j.contains("importance_trigger_curr"))
        cm.reflection_countdown_ = j["importance_trigger_curr"].get<int64_t>();

    // New format: "memories" array
    if (j.contains("memories")) {
        for (const auto& mj : j["memories"]) {
            MemoryNode node;
            node.id            = mj.value("id", std::uint64_t(0));
            node.type          = static_cast<MemoryType>(mj.value("type", 3));
            node.content       = sanitize_utf8(mj.value("content", ""));
            node.created_at    = mj.value("created_at", std::int64_t(0));
            node.weight        = mj.value("weight", 0.0f);
            node.depth         = mj.value("depth", short(0));
            node.last_accessed = mj.value("last_accessed", short(0));
            cm.add(std::move(node));
        }
    }
    // Old format: "beliefs" array
    else if (j.contains("beliefs")) {
        for (const auto& bj : j["beliefs"]) {
            MemoryNode node;
            node.id         = bj.value("id", std::uint64_t(0));
            node.type       = kEvent;
            node.content    = sanitize_utf8(bj.value("content", ""));
            node.created_at = bj.value("created_at", std::int64_t(0));
            node.weight     = bj.value("poignancy", 0.0f);
            node.depth      = bj.value("depth", short(0));
            cm.add(std::move(node));
        }
    }

    // Edges (same format in both old and new)
    if (j.contains("edges")) {
        for (const auto& ej : j["edges"]) {
            EdgeData ed;
            ed.weight     = ej.value("weight", 1.0f);
            ed.created_at = ej.value("created_at", 0);
            ed.active     = true;
            cm.add_edge(ej.value("from", std::uint64_t(0)),
                        ej.value("to",   std::uint64_t(0)),
                        ed);
        }
    }

    return cm;
}

}  // namespace rhapsode
