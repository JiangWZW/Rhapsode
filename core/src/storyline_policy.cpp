#include "rhapsode/storyline_policy.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "rhapsode/json_util.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

const std::string kSchedulerInstructions =
    "You are the scene scheduler for a parallel-storyline engine. The player's "
    "scene has just advanced. Up to TWO OFF-STAGE storylines may advance this "
    "turn while the others rest.\n"
    "Call list_scenes to see every live storyline. Weigh each off-stage row by "
    "its charge (how urgent its driving intention is), staleness (how many turns "
    "since it last advanced -- higher means overdue), and whether it is "
    "converging on the player. Drill in with query_graph or query_mind if a row "
    "is ambiguous.\n"
    "Then call advance_scene once per storyline you want advanced this turn "
    "(at most 2). Never pick the scene where player_present is true. Prefer the "
    "most overdue / highest-charge scenes. If nothing off-stage deserves a turn, "
    "call advance_scene once with an empty scene_id.";

const std::string kLifecycleInstructions =
    "You are the lifecycle director of a story that runs PARALLEL storylines. "
    "After EVERY scene step you review the WHOLE board and decide whether any "
    "live storyline should fork, merge, conclude, or exit characters.\n"
    "You receive: advanced_scene_id (the scene that just stepped), its full new "
    "narration/dialogue/player_action, and a row for EVERY live storyline "
    "(cast, intention, recent_narration).\n"
    "- MERGE: fold a NON-PLAYER storyline into another live one. "
    "{\"op\":\"merge\",\"from\":\"<scene_id>\",\"into\":\"<scene_id>\","
    "\"reason\":\"...\"}. "
    "from MUST NOT contain the Player. Prefer merge when board evidence shows "
    "co-presence (same place, same moment) -- including when the just-advanced "
    "scene's narration puts the Player with another storyline's cast, or when "
    "a fork's recent_narration shows the Player arriving. "
    "merge means retire `from` and move its cast into `into`.\n"
    "- CONCLUDE: a storyline's driving intention is fulfilled or dead -- end it. "
    "{\"op\":\"conclude\",\"scene_id\":\"...\",\"reason\":\"...\"}. "
    "Never conclude the storyline that contains the Player.\n"
    "- FORK: NON-PLAYER characters leave the JUST-ADVANCED scene with an ongoing "
    "goal. {\"op\":\"fork\",\"parent\":\"<advanced_scene_id>\",\"cast\":[names],"
    "\"driving_intention\":\"one sentence\"}. "
    "parent MUST equal advanced_scene_id. Never put Player in cast.\n"
    "- EXIT: a character leaves a storyline with no thread worth following. "
    "{\"op\":\"exit\",\"scene_id\":\"...\",\"names\":[names]}.\n"
    "Most steps change NOTHING. Return {\"ops\":[]} unless there is concrete "
    "evidence in this step's narration or the board rows.\n"
    "Use query_history, query_graph, query_mind, or list_scenes when needed.\n"
    "Reply with ONLY JSON:\n"
    "{\"ops\":[ ... ]}";

nlohmann::json scene_summary_json(const SceneSummary& summary) {
    // list_scenes / tool payload: keep last_narration short. recent_narration is
    // board-check only (see board_storyline_json).
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

nlohmann::json board_storyline_json(const SceneSummary& summary) {
    nlohmann::json row = scene_summary_json(summary);
    row["recent_narration"] = sanitize_utf8(summary.recent_narration);
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

int op_sort_key(LifecycleOp::Kind kind) {
    switch (kind) {
        case LifecycleOp::Kind::Merge: return 0;
        case LifecycleOp::Kind::Conclude: return 1;
        case LifecycleOp::Kind::Fork: return 2;
        case LifecycleOp::Kind::Exit: return 3;
    }
    return 9;
}

std::optional<LifecycleOp> parse_op(const nlohmann::json& element) {
    if (!element.is_object()) return std::nullopt;
    const std::string kind = str::trim(element.value("op", ""));
    LifecycleOp op;
    if (kind == "merge") {
        op.kind = LifecycleOp::Kind::Merge;
        op.from = str::trim(element.value("from", ""));
        op.into = str::trim(element.value("into", ""));
        op.reason = str::trim(element.value("reason", ""));
        if (op.from.empty() || op.into.empty() || op.from == op.into)
            return std::nullopt;
        return op;
    }
    if (kind == "conclude") {
        op.kind = LifecycleOp::Kind::Conclude;
        op.scene_id = str::trim(element.value("scene_id", ""));
        op.reason = str::trim(element.value("reason", ""));
        if (op.scene_id.empty() || op.reason.empty()) return std::nullopt;
        return op;
    }
    if (kind == "fork") {
        op.kind = LifecycleOp::Kind::Fork;
        op.scene_id = str::trim(element.value("parent", ""));
        op.cast = string_values(element.value("cast", nlohmann::json::array()));
        op.driving_intention = str::trim(element.value("driving_intention", ""));
        if (op.scene_id.empty() || op.cast.empty() ||
            op.driving_intention.empty()) {
            return std::nullopt;
        }
        std::unordered_set<std::string> seen;
        for (const auto& name : op.cast) {
            if (!seen.insert(str::to_lower(name)).second) return std::nullopt;
            if (str::iequals(name, "Player")) return std::nullopt;
        }
        return op;
    }
    if (kind == "exit") {
        op.kind = LifecycleOp::Kind::Exit;
        op.scene_id = str::trim(element.value("scene_id", ""));
        op.cast = string_values(element.value("names", nlohmann::json::array()));
        if (op.scene_id.empty() || op.cast.empty()) return std::nullopt;
        return op;
    }
    return std::nullopt;
}

}  // namespace

std::string serialize_scene_summaries(
    const std::vector<SceneSummary>& summaries) {
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& summary : summaries)
        rows.push_back(scene_summary_json(summary));
    return rows.dump();
}

std::string format_live_storylines_board(
    const std::vector<SceneSummary>& summaries) {
    if (summaries.empty()) return {};
    std::ostringstream os;
    os << "Other live threads:\n";
    for (const auto& summary : summaries) {
        os << "- ";
        if (summary.player_present)
            os << "This scene (" << summary.scene_id << "): you are here";
        else
            os << summary.scene_id;
        if (!summary.cast.empty()) {
            os << ". With ";
            for (size_t i = 0; i < summary.cast.size(); ++i) {
                if (i) os << ", ";
                os << summary.cast[i];
            }
        }
        if (!summary.driving_intention.empty())
            os << ". They want: " << summary.driving_intention;
        if (!summary.last_narration.empty())
            os << ". Last: " << summary.last_narration;
        os << ".\n";
    }
    return os.str();
}

std::string request_off_stage_scene(const SchedulerCallback& callback,
                                    const ReadToolCallback& read_tool) {
    if (!callback) return {};
    return str::trim(callback(
        kSchedulerInstructions,
        "Pick up to two off-stage scenes to advance (call advance_scene "
        "once per pick).",
        read_tool));
}

std::optional<LifecycleDecision> request_lifecycle_decision(
    const TurnSummary& summary, const LifecycleCallback& callback,
    const ReadToolCallback& read_tool) {
    if (!callback) return std::nullopt;

    nlohmann::json context;
    context["advanced_scene_id"] = sanitize_utf8(summary.scene_id);
    context["title"] = sanitize_utf8(summary.title);
    nlohmann::json cast = nlohmann::json::array();
    for (const auto& name : summary.cast)
        cast.push_back(sanitize_utf8(name));
    context["cast"] = std::move(cast);
    context["player_present"] = summary.player_present;
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
    for (const auto& storyline : summary.storylines)
        storylines.push_back(board_storyline_json(storyline));
    context["storylines"] = std::move(storylines);

    const std::string decide =
        "Decide lifecycle ops for the board after this step:\n" +
        context.dump(2);

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
    const auto ops_it = verdict.find("ops");
    if (ops_it == verdict.end()) {
        // Missing "ops" key: treat as a no-op (covers `{}` and `{"reasoning":...}`).
        return decision;
    }
    if (!ops_it->is_array()) return std::nullopt;

    for (const auto& element : *ops_it) {
        auto op = parse_op(element);
        if (!op) return std::nullopt;
        decision.ops.push_back(std::move(*op));
    }

    std::stable_sort(decision.ops.begin(), decision.ops.end(),
        [](const LifecycleOp& a, const LifecycleOp& b) {
            return op_sort_key(a.kind) < op_sort_key(b.kind);
        });
    return decision;
}

std::string build_autonomous_cue(const SceneSummary& summary) {
    std::string cue =
        "[Off-stage step: the player is elsewhere. Advance THIS storyline by one "
        "meaningful step toward its goal, and record what changes as facts.]";
    if (!summary.driving_intention.empty())
        cue += "\nDriving intention: " + summary.driving_intention;
    if (!summary.last_narration.empty())
        cue += "\nLast we saw: " + summary.last_narration;
    cue += "\nIf this storyline's cast has physically reached another storyline's "
           "location (co-presence, not mere pursuit), author that arrival in this "
           "step. Do not call tools to merge; the board lifecycle check decides "
           "afterward.";
    return cue;
}

}  // namespace rhapsode
