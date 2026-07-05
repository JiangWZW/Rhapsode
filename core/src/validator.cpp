#include "rhapsode/validator.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace rhapsode {

Validator::Validator(const WorldGraph& graph) : graph_(graph) {}

void Validator::set_llm_callback(ValidatorLLMCallback cb) { llm_cb_ = std::move(cb); }
void Validator::set_search_callback(SearchCallback cb) { search_cb_ = std::move(cb); }
void Validator::set_dead_check(DeadCheckCallback cb) { dead_check_cb_ = std::move(cb); }

// ---------------------------------------------------------------------------
// Context gathering: entity chain + ChromaDB, structural valid_until filter
// ---------------------------------------------------------------------------

std::vector<const Node*> Validator::gather_context(const Node& candidate) const {
    static constexpr int kChainDepth = 3;
    static constexpr int kChromaHits = 5;

    std::unordered_set<std::uint64_t> seen;
    std::vector<const Node*> context;

    auto dominated = [&](const Node* n) {
        return n->valid_until != -1 && n->valid_until <= candidate.created_at;
    };

    std::unordered_set<std::string> target_entities;
    for (const auto& e : candidate.entities)
        target_entities.insert(str::to_lower(e));

    int chain_total = 0;
    for (const auto& entity : target_entities) {
        std::vector<const Node*> matches;
        graph_.for_each([&](const Node& n) {
            if (n.state == NodeState::Dormant) return;  // live = active or foreshadowed
            if (dominated(&n)) return;
            for (const auto& ne : n.entities) {
                if (str::to_lower(ne) == entity) {
                    matches.push_back(&n);
                    return;
                }
            }
        }, true);

        std::sort(matches.begin(), matches.end(),
            [](const Node* a, const Node* b) { return a->created_at < b->created_at; });

        int start = std::max(0, static_cast<int>(matches.size()) - kChainDepth);
        int added = 0;
        for (int i = start; i < static_cast<int>(matches.size()); ++i) {
            if (seen.insert(matches[i]->id).second) {
                context.push_back(matches[i]);
                ++added;
            }
        }
        chain_total += added;
        log() << "  [validator] context: entity \"" << entity
              << "\" -> " << matches.size() << " match(es), "
              << added << " added\n";
    }

    int chroma_added = 0;
    if (search_cb_) {
        auto hits = search_cb_(candidate.fact, kChromaHits);
        for (auto id : hits) {
            const Node* n = graph_.get_node(id);
            if (n && n->state != NodeState::Dormant
                && !dominated(n) && seen.insert(n->id).second) {
                context.push_back(n);
                ++chroma_added;
            }
        }
        log() << "  [validator] context: ChromaDB -> "
              << hits.size() << " hit(s), " << chroma_added << " new\n";
    }

    std::sort(context.begin(), context.end(),
        [](const Node* a, const Node* b) { return a->created_at < b->created_at; });

    log() << "  [validator] context: " << context.size()
          << " total (" << chain_total << " chain + "
          << chroma_added << " chroma)\n";

    return context;
}

// ---------------------------------------------------------------------------
// Static boilerplate — shared by full and compact prompt tiers
// ---------------------------------------------------------------------------

namespace {

const std::string kHeader =
    "You are a continuity editor for an interactive story. "
    "Your job is to check whether a new event contradicts established facts.\n\n";

const std::string kExamples =
    "=== EXAMPLES ===\n\n"

    "Example 1 (dead character acts):\n"
    "Dead: Captain Reed\n"
    "Chain: (empty)\n"
    "New: \"Reed addresses the squad from the podium\"\n"
    "Answer: {\"contradicts\": true, \"reasoning\": \"Reed is dead. "
    "Dead characters cannot speak or appear.\"}\n\n"

    "Example 2 (a living character can be killed):\n"
    "Dead: (none)\n"
    "Chain:\n"
    "  (turn 2) [active] \"The guard is at the gate\"\n"
    "New: \"An accident kills the guard\"\n"
    "Answer: {\"contradicts\": false, \"reasoning\": \"The guard is alive "
    "and present. A living character can be killed.\"}\n\n"

    "Example 3 (reuse of a destroyed thing):\n"
    "Chain:\n"
    "  (turn 3) [active] \"The guild hall collapsed into rubble\"\n"
    "New: \"The party meets inside the guild hall\"\n"
    "Answer: {\"contradicts\": true, \"reasoning\": \"The guild hall was destroyed "
    "and no rebuilding event appears in the chain.\"}\n\n"

    "Example 4 (a foreshadowed payoff is expected, not a contradiction):\n"
    "Chain:\n"
    "  (turn 0) [foreshadowed] \"A rival faction plans a surprise attack\"\n"
    "New: \"The rival faction launches the surprise attack\"\n"
    "Answer: {\"contradicts\": false, \"reasoning\": \"The attack is already "
    "planted as foreshadowed. It happening is the expected payoff.\"}\n\n";

const std::string kFooter =
    "=== YOUR TASK ===\n"
    "The chain is the current world state. [active] = true right now. "
    "[foreshadowed] = planted and on its way to becoming true.\n\n"

    "Your only job is to catch impossibilities. New facts, new conditions, and "
    "foreshadowed payoffs are expected — they are how the story advances, not "
    "contradictions. Do NOT reject an event merely for being new or unestablished.\n\n"

    "REJECT only when:\n"
    "- A dead character (see the dead list) acts, speaks, or appears.\n"
    "- The event reuses or reverses something permanently ended — destruction, "
    "death, or consumption — with no restoration event in the chain.\n\n"

    "Otherwise ACCEPT. If unsure, ACCEPT.\n"
    "Answer JSON: {\"contradicts\": true/false, \"reasoning\": \"...\"}\n";

const size_t kFullBoilerplate =
    kHeader.size() + kExamples.size() + kFooter.size();

constexpr size_t kSafePromptChars = 10000;

}  // anonymous namespace

// ---------------------------------------------------------------------------
// Prompt data section: the variable part (chain + dead list + new event)
// ---------------------------------------------------------------------------

std::string Validator::build_data_section(
        const Node& candidate,
        const std::vector<const Node*>& context,
        const std::vector<std::string>& dead_entities) const {
    std::ostringstream os;

    os << "=== WORLD STATE ===\n";
    if (!dead_entities.empty()) {
        os << "Dead characters:";
        for (const auto& d : dead_entities) os << " " << d;
    } else {
        os << "Dead characters: (none)";
    }
    os << "\n\n";

    os << "=== ESTABLISHED CHAIN (oldest to newest) ===\n";
    for (const auto* n : context) {
        os << "- (turn " << n->created_at << ") "
           << "[" << to_string(n->state) << "] "
           << "\"" << n->fact << "\"\n";
    }

    os << "\n=== NEW EVENT ===\n\"" << candidate.fact << "\"\n";
    return os.str();
}

// ---------------------------------------------------------------------------
// LLM call helper
// ---------------------------------------------------------------------------

std::string Validator::try_llm_call(const std::string& prompt) const {
    try {
        return llm_cb_(sanitize_utf8(prompt));
    } catch (const std::exception& e) {
        log() << "  [validator] LLM call failed: " << e.what()
              << " -- accepting by default\n";
        return "";
    }
}

// ---------------------------------------------------------------------------
// Main check — adaptive prompt: strip examples when data section is large
// ---------------------------------------------------------------------------

Verdict Validator::check(const Node& candidate) const {
    if (!llm_cb_) return {true, ""};

    log() << "  [validator] checking \"" << candidate.fact << "\"\n";

    auto dead = dead_check_cb_ ? dead_check_cb_() : std::vector<std::string>{};
    auto context = gather_context(candidate);
    if (context.empty()) {
        log() << "  [validator] no context -- auto-accept\n";
        return {true, ""};
    }

    auto data = build_data_section(candidate, context, dead);
    bool compact = data.size() + kFullBoilerplate > kSafePromptChars;

    // Examples BEFORE data so the model learns the pattern first.
    // Large chains drop the examples to stay within the prompt budget.
    std::string prompt = compact
        ? kHeader + data + kFooter
        : kHeader + kExamples + data + kFooter;

    log() << "  [validator] prompt: " << prompt.size() << " chars ("
          << (compact ? "compact" : "full") << ")\n" << std::flush;

    auto response = try_llm_call(prompt);
    if (response.empty()) {
        log() << "  [validator] empty response -- accepting by default\n";
        return {true, ""};
    }

    log() << "  [validator] raw response: " << response << "\n";

    // Parse with status logging
    nlohmann::json j;
    bool parsed = false;
    try {
        j = nlohmann::json::parse(response);
        parsed = true;
        log() << "  [validator] parse: OK (direct)\n";
    } catch (...) {
        std::string salvaged;
        if (extract_balanced_json(response, salvaged)) {
            try {
                j = nlohmann::json::parse(salvaged);
                parsed = true;
                log() << "  [validator] parse: OK (extracted from \""
                      << salvaged.substr(0, 100) << "\")\n";
            } catch (...) {}
        }
        if (!parsed) {
            log() << "  [validator] parse: FAILED -- defaulting to accept\n";
            return {true, ""};
        }
    }

    bool contradicts = j.value("contradicts", false);
    std::string reason = j.value("reason", j.value("reasoning", ""));

    log() << "  [validator] verdict: contradicts="
          << (contradicts ? "true" : "false")
          << " reasoning=\"" << reason << "\"\n";

    if (contradicts) {
        log() << "  [validator] REJECTED\n";
        return {false, reason};
    }

    log() << "  [validator] accepted\n";
    return {true, ""};
}

}  // namespace rhapsode
