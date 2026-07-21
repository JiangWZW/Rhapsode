#include "rhapsode/character_memory.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace rhapsode {

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

void CharacterMemory::reflect_perceptions(int turn, const std::string& description,
                                          const LLMCallback& llm_callback) {
    if (!llm_callback) {
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
        raw = llm_callback(prompt);
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

}  // namespace rhapsode
