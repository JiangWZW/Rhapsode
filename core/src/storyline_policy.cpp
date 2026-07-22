#include "rhapsode/storyline_policy.h"

#include <nlohmann/json.hpp>

#include <unordered_set>

#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

const std::string kSchedulerInstructions =
    "You are the scene scheduler for a parallel-storyline engine. The player's "
    "scene has just advanced. Exactly one OFF-STAGE storyline may advance this "
    "turn while the others rest.\n"
    "Call list_scenes to see every live storyline. Weigh each off-stage row by "
    "its charge (how urgent its driving intention is), staleness (how many beats "
    "since it last advanced -- higher means overdue), and whether it is "
    "converging on the player. Drill in with query_graph or query_mind if a row "
    "is ambiguous.\n"
    "Then call advance_scene exactly once with the scene_id of the single most "
    "deserving off-stage storyline. Never pick the scene where player_present is "
    "true. If nothing off-stage deserves a beat, call advance_scene with an empty "
    "scene_id.";

const std::string kLifecycleInstructions =
    "You are the lifecycle director of a story that runs PARALLEL storylines. "
    "After each beat you decide whether that beat changed the set of live "
    "storylines for the scene just advanced.\n"
    "- FORK: one or more NON-PLAYER characters leave the scene carrying an ongoing "
    "goal or unresolved stake -- a party splits, a subgroup is dispatched, someone "
    "storms off with a purpose, or the player walks away leaving companions behind "
    "(fork the ones LEFT BEHIND). The player NEVER leaves their own storyline; "
    "never put \"Player\" in a fork.\n"
    "- MERGE: this scene's cast has physically reunited with another live "
    "storyline (co-presence evidenced in THIS beat's narration, not mere "
    "pursuit) -- fold this scene into that one via merge_into set to that "
    "storyline's scene_id from other_storylines.\n"
    "- CONCLUDE: this scene's driving intention is fulfilled or dead -- end it.\n"
    "- EXIT: a character simply leaves with no ongoing thread worth following (a "
    "passing NPC the group walked away from). They leave the cast; no new "
    "storyline.\n"
    "If the cast includes \"Player\", this is the player's OWN storyline: it can "
    "never be concluded or merged away -- the player is always in it, so it always "
    "continues. Only a fork (companions leaving) or an exit may apply to it; never "
    "conclude or merge it even if a shared goal just ended.\n"
    "Most beats change NOTHING. Act only on a concrete change in THIS beat -- never "
    "on mood, a passing aside, or characters merely staying together.\n"
    "Use query_history, query_graph, or query_mind when the beat does not contain "
    "enough evidence to decide.\n"
    "Reply with ONLY JSON (use null / [] for anything that does not apply):\n"
    "{\"fork\": {\"cast\": [names], \"driving_intention\": \"one sentence\"} | null, "
    "\"merge_into\": \"scene_id\" | null, \"conclude\": \"reason\" | null, "
    "\"exited\": [names]}";

nlohmann::json scene_summary_json(const SceneSummary& summary) {
    nlohmann::json row;
    row["scene_id"] = summary.scene_id;
    row["title"] = summary.title;
    row["active"] = summary.active;
    row["turn_index"] = summary.turn_index;
    row["staleness"] = summary.staleness;
    row["cast"] = summary.cast;
    row["player_present"] = summary.player_present;
    row["driving_intention"] = summary.driving_intention;
    row["charge"] = summary.charge;
    row["last_narration"] = summary.last_narration;
    return row;
}

std::vector<std::string> string_values(const nlohmann::json& values) {
    std::vector<std::string> result;
    if (!values.is_array()) return result;
    for (const auto& value : values) {
        if (value.is_string()) result.push_back(value.get<std::string>());
    }
    return result;
}

}  // namespace

std::string serialize_scene_summaries(
    const std::vector<SceneSummary>& summaries) {
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& summary : summaries)
        rows.push_back(scene_summary_json(summary));
    return rows.dump();
}

std::string request_off_stage_scene(const SchedulerCallback& callback,
                                    const ReadToolCallback& read_tool) {
    if (!callback) return {};
    return str::trim(callback(
        kSchedulerInstructions, "Pick the next off-stage scene to advance.",
        read_tool));
}

std::optional<LifecycleDecision> request_lifecycle_decision(
    const BeatSummary& summary, const LifecycleCallback& callback,
    const ReadToolCallback& read_tool) {
    if (!callback) return std::nullopt;

    nlohmann::json context;
    context["scene_id"] = summary.scene_id;
    context["title"] = summary.title;
    context["cast"] = summary.cast;
    context["player_action"] = summary.player_action
        ? nlohmann::json(*summary.player_action) : nlohmann::json(nullptr);
    context["narration"] = summary.narration;
    context["dialogue"] = nlohmann::json::array();
    for (const auto& message : summary.dialogue) {
        nlohmann::json row{{"content", message.content}};
        if (message.metadata.contains("speaker"))
            row["speaker"] = message.metadata["speaker"];
        context["dialogue"].push_back(std::move(row));
    }
    nlohmann::json storylines = nlohmann::json::array();
    for (const auto& storyline : summary.storylines) {
        if (storyline.scene_id == summary.scene_id) continue;
        storylines.push_back(scene_summary_json(storyline));
    }
    context["other_storylines"] = std::move(storylines);

    const std::string raw = callback(
        kLifecycleInstructions,
        "Decide the lifecycle verdict for the beat just authored:\n" +
            context.dump(2),
        read_tool);
    const auto left = raw.find('{');
    const auto right = raw.rfind('}');
    if (left == std::string::npos || right == std::string::npos || right < left)
        return std::nullopt;

    const auto verdict = nlohmann::json::parse(
        raw.substr(left, right - left + 1), nullptr, false);
    if (!verdict.is_object()) return std::nullopt;

    LifecycleDecision decision;
    if (const auto it = verdict.find("conclude");
        it != verdict.end() && it->is_string() && !it->get<std::string>().empty()) {
        decision.conclude_reason = it->get<std::string>();
    }
    if (const auto it = verdict.find("merge_into");
        it != verdict.end() && it->is_string() && !it->get<std::string>().empty()) {
        decision.merge_into = it->get<std::string>();
    }
    if (const auto it = verdict.find("fork");
        it != verdict.end() && it->is_object()) {
        LifecycleDecision::Fork fork;
        fork.cast = string_values(it->value("cast", nlohmann::json::array()));
        fork.driving_intention = str::trim(
            it->value("driving_intention", ""));
        decision.fork = std::move(fork);
    }
    if (const auto it = verdict.find("exited"); it != verdict.end())
        decision.exited = string_values(*it);

    const int terminal_actions =
        static_cast<int>(decision.conclude_reason.has_value()) +
        static_cast<int>(decision.merge_into.has_value());
    if (terminal_actions > 1 ||
        (terminal_actions == 1 &&
         (decision.fork.has_value() || !decision.exited.empty()))) {
        return std::nullopt;
    }
    if (decision.fork) {
        if (decision.fork->cast.empty() ||
            decision.fork->driving_intention.empty()) {
            return std::nullopt;
        }
        std::unordered_set<std::string> forked;
        for (const auto& name : decision.fork->cast) {
            if (!forked.insert(str::to_lower(name)).second)
                return std::nullopt;
        }
        for (const auto& name : decision.exited) {
            if (forked.count(str::to_lower(name)) > 0)
                return std::nullopt;
        }
    }
    return decision;
}

std::string build_autonomous_cue(const SceneSummary& summary) {
    std::string cue =
        "[Off-stage beat: the player is elsewhere. Advance THIS storyline by one "
        "meaningful step toward its goal, and record what changes as facts.]";
    if (!summary.driving_intention.empty())
        cue += "\nDriving intention: " + summary.driving_intention;
    if (!summary.last_narration.empty())
        cue += "\nLast we saw: " + summary.last_narration;
    cue += "\nIf this storyline's cast has physically reached another storyline's "
           "location (co-presence, not mere pursuit), author that arrival in this "
           "beat. Do not call tools to merge; lifecycle decides afterward.";
    return cue;
}

}  // namespace rhapsode
