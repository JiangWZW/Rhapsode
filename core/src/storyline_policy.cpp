#include "rhapsode/storyline_policy.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <unordered_set>

#include "rhapsode/json_util.h"
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
    "- MERGE: retire THIS scene by folding it into another live storyline. Set "
    "merge_into to that storyline's scene_id from other_storylines. "
    "merge_into ALWAYS means \"end the scene you are judging now and move its cast "
    "into the target\"; it NEVER means \"absorb the other scene into this one\" "
    "and it NEVER means \"send the player into a fork.\" "
    "Require physical co-presence in THIS beat's narration (same place, same "
    "moment) -- not pursuit, not shouting across a distance, not merely knowing "
    "where the others are.\n"
    "- CONCLUDE: this scene's driving intention is fulfilled or dead -- end it.\n"
    "- EXIT: a character simply leaves with no ongoing thread worth following (a "
    "passing NPC the group walked away from). They leave the cast; no new "
    "storyline.\n"
    "If the cast includes \"Player\", this is the player's OWN storyline:\n"
    "- merge_into MUST be null. Always. Even when this beat's narration shows the "
    "player reuniting with an off-stage cast -- do NOT merge the player scene into "
    "a fork. The off-stage storyline merges into the player scene when THAT scene "
    "advances.\n"
    "- conclude MUST be null. The player storyline always continues.\n"
    "- Only fork (companions leaving) or exit may apply.\n"
    "If the cast does NOT include \"Player\":\n"
    "- When THIS beat shows co-presence with the player storyline (other_storylines "
    "row with player_present=true, or narration shows the Player arriving / standing "
    "with this cast), set merge_into to that player storyline's scene_id. Prefer "
    "merge over forking further or continuing alone.\n"
    "- The same rule applies when reuniting with any other non-player storyline: "
    "fold THIS scene into theirs via merge_into, never the reverse from a scene "
    "you are not judging.\n"
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
    row["scene_id"] = sanitize_utf8(summary.scene_id);
    row["title"] = sanitize_utf8(summary.title);
    row["active"] = summary.active;
    row["turn_index"] = summary.turn_index;
    row["staleness"] = summary.staleness;
    nlohmann::json cast = nlohmann::json::array();
    for (const auto& name : summary.cast)
        cast.push_back(sanitize_utf8(name));
    row["cast"] = std::move(cast);
    row["player_present"] = summary.player_present;
    row["driving_intention"] = sanitize_utf8(summary.driving_intention);
    row["charge"] = summary.charge;
    row["last_narration"] = sanitize_utf8(summary.last_narration);
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
    context["scene_id"] = sanitize_utf8(summary.scene_id);
    context["title"] = sanitize_utf8(summary.title);
    nlohmann::json cast = nlohmann::json::array();
    for (const auto& name : summary.cast)
        cast.push_back(sanitize_utf8(name));
    context["cast"] = std::move(cast);
    context["player_action"] = summary.player_action
        ? nlohmann::json(sanitize_utf8(*summary.player_action))
        : nlohmann::json(nullptr);
    context["narration"] = sanitize_utf8(summary.narration);
    context["dialogue"] = nlohmann::json::array();
    for (const auto& message : summary.dialogue) {
        nlohmann::json row{{"content", sanitize_utf8(message.content)}};
        if (message.metadata.contains("speaker") &&
            message.metadata["speaker"].is_string()) {
            row["speaker"] = sanitize_utf8(
                message.metadata["speaker"].get<std::string>());
        }
        context["dialogue"].push_back(std::move(row));
    }
    nlohmann::json storylines = nlohmann::json::array();
    for (const auto& storyline : summary.storylines) {
        if (storyline.scene_id == summary.scene_id) continue;
        storylines.push_back(scene_summary_json(storyline));
    }
    context["other_storylines"] = std::move(storylines);

    std::string decide =
        "Decide the lifecycle verdict for the beat just authored:\n" +
        context.dump(2);
    const bool player_scene = std::any_of(
        summary.cast.begin(), summary.cast.end(),
        [](const std::string& name) { return str::iequals(name, "Player"); });
    if (player_scene) {
        decide += "\n\nReminder: cast includes Player — merge_into and conclude "
                  "MUST be null. Reunion with a fork is handled when that fork "
                  "advances (it merges into this scene).";
    } else {
        decide += "\n\nReminder: non-player scene — if THIS beat shows co-presence "
                  "with the player storyline, set merge_into to that scene_id "
                  "(fold THIS scene into theirs). Never invent the reverse.";
    }

    const std::string raw = callback(
        kLifecycleInstructions, decide, read_tool);
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
           "beat. If the Player has arrived and is with this cast, make that reunion "
           "explicit in the narration so lifecycle can merge THIS scene into the "
           "player's storyline (never the reverse). Do not call tools to merge; "
           "lifecycle decides afterward.";
    return cue;
}

}  // namespace rhapsode
