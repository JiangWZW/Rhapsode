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

struct Perception {
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
    std::size_t bucket_index;
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

std::vector<Perception> gather_unreflected_perceptions(
    const WorldGraph& beliefs) {
    std::vector<Perception> perceptions;
    beliefs.for_each([&](const Node& n) {
        if (n.type == "perception" && n.state == NodeState::Active)
            perceptions.push_back({n.id, n.fact, n.entities});
    }, false);
    return perceptions;
}

// Entity-less perceptions fold under a synthetic "(world)" subject so general
// news still becomes belief.
std::unordered_map<std::string, std::vector<std::size_t>>
group_perceptions_by_subject(const std::vector<Perception>& perceptions) {
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
    const std::vector<Perception>& perceptions,
    const std::unordered_map<std::string, std::vector<std::size_t>>& by_subject)
{
    std::vector<ReflectionBucket> buckets;
    buckets.reserve(by_subject.size());
    for (const auto& [subject, perception_indices] : by_subject) {
        ReflectionBucket bucket;
        bucket.subject = subject;
        bucket.perception_idxs = perception_indices;
        const std::string subject_key = str::to_lower(subject);
        beliefs.for_each([&](const Node& n) {
            if (n.type != "belief" || n.state != NodeState::Active) return;
            for (const auto& e : n.entities) {
                if (entity_matches(str::to_lower(e), subject_key)) {
                    bucket.prior_ids.push_back(n.id);
                    bucket.prior_text += "- " + n.fact + "\n";
                    return;
                }
            }
        }, false);
        buckets.push_back(std::move(bucket));
    }
    return buckets;
}

std::string build_reflection_prompt(
    const std::string& character_name,
    const std::string& description,
    const std::vector<ReflectionBucket>& buckets,
    const std::vector<Perception>& perceptions)
{
    std::string prompt =
        "You are " + character_name + ".\n"
        + (description.empty() ? std::string() : "Who I am: " + description + "\n") +
        "I have just perceived new things about several subjects. For EACH "
        "subject below, in ONE terse first-person sentence -- how I would "
        "actually think it, not prose -- say what this now makes me think or "
        "feel about it. Do not tidy it: if it sits uneasily with what I already "
        "think, let it. Rate how much it presses on me from 1 (settled or "
        "trivial) to 10 (unbearably charged). Say whether the new thought "
        "CONTRADICTS what I already thought (\"tension\") or EXTENDS / supports "
        "it (\"evidence\").\n\nSubjects:\n";
    for (std::size_t bucket_index = 0;
         bucket_index < buckets.size(); ++bucket_index) {
        const auto& bucket = buckets[bucket_index];
        prompt += "[" + std::to_string(bucket_index) + "] "
               + bucket.subject + "\n";
        if (bucket.prior_text.empty())
            prompt += "  What I already think: (nothing in particular yet)\n";
        else
            prompt += "  What I already think:\n" + bucket.prior_text;
        prompt += "  What I just perceived:\n";
        for (const auto perception_index : bucket.perception_idxs)
            prompt += "  - " + perceptions[perception_index].fact + "\n";
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
    for (const auto& value : *it) {
        if (!value.is_object()) continue;
        const std::size_t bucket_index = static_cast<std::size_t>(
            json_number<int>(value, "id", -1));
        if (bucket_index >= bucket_count) continue;
        std::string belief =
            strip_echoed_label(sanitize_utf8(value.value("thought", "")));
        if (belief.empty()) continue;
        const int weight =
            std::clamp(json_number<int>(value, "weight", 5), 1, 10);
        const std::string relation =
            str::to_lower(value.value("relation", "evidence"));
        const std::string kind =
            (relation.find("tension") != std::string::npos ||
             relation.find("contradict") != std::string::npos)
                ? "tension" : "evidence";
        thoughts.push_back(
            {bucket_index, std::move(belief), weight, kind});
    }
    return thoughts;
}

// Creates belief nodes + tension/evidence edges to priors and perceptions.
std::vector<std::uint64_t> apply_reflection_thoughts(
    WorldGraph& beliefs,
    int turn,
    const std::vector<ReflectionBucket>& buckets,
    const std::vector<Perception>& perceptions,
    const std::vector<ParsedThought>& thoughts)
{
    std::vector<std::uint64_t> new_thought_ids;
    for (const auto& thought : thoughts) {
        const ReflectionBucket& bucket = buckets[thought.bucket_index];
        Node belief;
        belief.fact = sanitize_utf8(thought.belief);
        belief.type = "belief";
        belief.state = NodeState::Active;
        belief.entities = (bucket.subject == "(world)")
                             ? std::vector<std::string>{}
                             : std::vector<std::string>{bucket.subject};
        belief.created_at = turn;
        belief.valid_until = -1;
        belief.weight = static_cast<float>(thought.weight);
        const std::uint64_t new_id = beliefs.add_node(std::move(belief)).id;
        new_thought_ids.push_back(new_id);

        for (const auto prior_id : bucket.prior_ids)
            beliefs.add_relation(
                new_id, prior_id, 1.0f, turn, thought.kind);
        for (const auto perception_index : bucket.perception_idxs)
            beliefs.add_relation(new_id, perceptions[perception_index].id,
                                 1.0f, turn, "evidence");
    }
    return new_thought_ids;
}

// Mark consolidated perceptions as no longer live (kept as history, not
// re-reflected next turn).
void consolidate_perceptions(WorldGraph& beliefs,
                             const std::vector<Perception>& perceptions,
                             int turn)
{
    for (const auto& perception : perceptions)
        beliefs.set_valid_until(perception.id, turn);
}

// Weight lives by reinforce-vs-decay (the only writer of Thought weight).
// Each new Thought touches its local neighborhood and reinforces it; Thoughts
// left untouched this pass decay.  Below the cull floor a Thought is retired
// from the live pool.  Returns the cull count.
int apply_reinforce_decay(WorldGraph& beliefs, int turn,
                          const std::vector<std::uint64_t>& new_thought_ids,
                          const WeightTuning& tuning,
    const std::string& character_name)
{
    std::unordered_set<std::uint64_t> touched;
    for (const auto new_thought_id : new_thought_ids) {
        touched.insert(new_thought_id);
        for (const auto neighbor_id :
             beliefs.neighbors_within(new_thought_id, tuning.touch_hops)) {
            touched.insert(neighbor_id);
            Node* neighbor = beliefs.get_node(neighbor_id);
            if (neighbor && neighbor->type == "belief") {
                neighbor->weight = std::min(
                    tuning.weight_cap,
                    neighbor->weight + tuning.reinforce);
            }
        }
    }
    std::vector<std::uint64_t> live_thoughts;
    beliefs.for_each([&](const Node& n) {
        if (n.type == "belief" && n.state == NodeState::Active)
            live_thoughts.push_back(n.id);
    }, false);
    int culled = 0;
    for (const auto belief_id : live_thoughts) {
        if (touched.count(belief_id)) continue;
        Node* belief = beliefs.get_node(belief_id);
        if (!belief) continue;
        belief->weight *= tuning.decay;
        if (belief->weight < tuning.cull_floor) {
            beliefs.set_valid_until(belief_id, turn);
            ++culled;
            log() << "  [char_mem:" << character_name << "] culled #"
                  << belief_id << " (w=" << belief->weight << "): "
                  << belief->fact << "\n";
        }
    }
    return culled;
}

}  // namespace

void CharacterMemory::reflect_perceptions(int turn, const std::string& description,
                                          const LLMCallback& llm_callback) {
    if (!llm_callback) {
        log() << "  [char_mem:" << character_name_
              << "] reflect skip: no reflection LLM callback\n" << std::flush;
        return;
    }

    auto perceptions = gather_unreflected_perceptions(beliefs_);
    if (perceptions.empty()) {
        log() << "  [char_mem:" << character_name_
              << "] reflect skip: no new perceptions\n" << std::flush;
        return;
    }

    auto by_subject = group_perceptions_by_subject(perceptions);
    auto buckets    = build_reflection_buckets(beliefs_, perceptions, by_subject);
    auto prompt     = build_reflection_prompt(
        character_name_, description, buckets, perceptions);

    std::string raw;
    try {
        raw = llm_callback(prompt);
    } catch (const std::exception& ex) {
        log() << "  [char_mem:" << character_name_
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
        beliefs_, turn, new_thought_ids, WeightTuning{}, character_name_);

    log() << "  [char_mem:" << character_name_ << "] reflected "
          << perceptions.size() << " perception(s) into "
          << new_thought_ids.size() << " thought(s), culled "
          << culled << " faded\n" << std::flush;
}

}  // namespace rhapsode
