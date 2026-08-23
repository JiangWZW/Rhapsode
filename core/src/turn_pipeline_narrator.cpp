#include "rhapsode/turn_pipeline.h"

#include "turn_pipeline_internal.h"

#include "rhapsode/character.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/str_util.h"
#include "rhapsode/world.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>

namespace rhapsode {
namespace {

constexpr char kJsonToken[] = "<<<RHAPSODE_JSON>>>";
constexpr int kMaxNarratorAttempts = 3;

bool cast_name_matches(const std::string& cast_name, const std::string& character_name) {
    const auto cast_lower = str::to_lower(cast_name);
    const auto character_lower = str::to_lower(character_name);
    if (cast_lower == character_lower) return true;
    return str::has_word_match(cast_lower, character_lower) ||
           str::has_word_match(character_lower, cast_lower);
}

const Character* resolve_cast_name(const std::string& cast_name,
                                   const std::vector<Character>& characters) {
    const Character* best = nullptr;
    for (const auto& ch : characters) {
        if (ch.is_player || !cast_name_matches(cast_name, ch.name)) {
            continue;
        }

        if (!best || str::iequals(cast_name, ch.name)) {
            best = &ch;
            if (str::iequals(cast_name, ch.name)) {
                break;
            }
        }
    }
    return best;
}

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw) {
    raw = normalize_json_punct(std::move(raw));

    const auto marker = raw.find(kJsonToken);
    if (marker != std::string::npos) {
        auto prose = str::trim(raw.substr(0, marker));
        auto json = str::trim(raw.substr(marker + std::strlen(kJsonToken)));
        return {std::move(prose),
                json.empty() ? nlohmann::json::object() : try_parse_json(json)};
    }

    static constexpr std::array<const char*, 5> kPlanKeys = {
        "transitions", "new_nodes", "speech_turns", "active_cast", "new_characters"};

    for (auto brace = raw.find('{'); brace != std::string::npos;
         brace = raw.find('{', brace + 1)) {
        std::string fragment;
        if (!extract_balanced_json(std::string_view(raw).substr(brace), fragment)) {
            break;
        }
        try {
            auto plan = nlohmann::json::parse(fragment);
            if (plan.is_object() &&
                std::any_of(kPlanKeys.begin(), kPlanKeys.end(),
                            [&](const char* k) { return plan.contains(k); })) {
                return {str::trim(raw.substr(0, brace)), std::move(plan)};
            }
        } catch (...) {
        }
    }

    return {str::trim(std::move(raw)), nlohmann::json::object()};
}

std::vector<Rejection> validate_active_cast(const nlohmann::json& plan,
                                            const std::vector<Character>& characters) {
    std::vector<Rejection> rejections;
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) {
        return rejections;
    }

    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) {
            continue;
        }
        auto name = elem.get<std::string>();
        const Character* ch = resolve_cast_name(name, characters);
        if (!ch) {
            log() << "  [cast] active_cast lists unresolved \"" << name << "\" -- ignoring\n";
        } else if (ch->dead) {
            rejections.push_back({"active_cast includes \"" + name + "\"", "character is dead"});
        }
    }
    return rejections;
}

std::vector<Rejection> validate_speech_turns(const nlohmann::json& plan,
                                             const std::vector<Character>& characters) {
    std::vector<Rejection> rejections;
    if (!plan.contains("speech_turns") || !plan["speech_turns"].is_array())
        return rejections;

    auto names_player = [&](const std::string& name) {
        if (str::to_lower(name) == "player") return true;
        for (const auto& ch : characters) {
            if (ch.is_player && resolve_cast_name(name, characters) == &ch)
                return true;
        }
        return false;
    };

    const auto& turns = plan["speech_turns"];
    int npc_cues = 0;
    for (const auto& el : turns) {
        if (!el.is_object()) continue;
        const auto name = el.value("character", "");
        if (name.empty()) continue;
        if (names_player(name)) {
            rejections.push_back({
                "speech_turns includes \"" + name + "\"",
                "Player must not appear in speech_turns; the user message is the "
                "player's speech -- give responding NPCs their own speech_turn entries"
            });
            continue;
        }
        const Character* ch = resolve_cast_name(name, characters);
        if (ch && !ch->dead) ++npc_cues;
    }

    if (turns.empty()) return rejections;

    int speakable = 0;
    if (plan.contains("active_cast") && plan["active_cast"].is_array()) {
        for (const auto& elem : plan["active_cast"]) {
            if (!elem.is_string()) continue;
            const Character* ch =
                resolve_cast_name(elem.get<std::string>(), characters);
            if (ch && !ch->dead) ++speakable;
        }
    }
    if (speakable > 0 && npc_cues == 0) {
        rejections.push_back({
            "speech_turns",
            "NPCs are present in active_cast but no NPC speech_turns were authored "
            "(speech_turns must contain each speaking NPC's line, not the Player's)"
        });
    }
    return rejections;
}

std::string build_revision_turn_state(
    const std::string& original,
    const std::vector<Rejection>& rejections) {
    std::string revision = original;
    revision += "\n\n### REVISION REQUIRED\n"
                "The following issues were found in your plan:\n";
    for (const auto& rejection : rejections)
        revision += "- " + rejection.fact + " -- " + rejection.reason + "\n";
    revision += "\nRewrite your narrative and plan to fix these issues.\n";
    return revision;
}

std::string build_graph_turn_state(const std::string& prose,
                                   const nlohmann::json& turn_plan) {
    std::ostringstream os;
    if (turn_plan.is_object()) {
        const auto cast = turn_plan.find("active_cast");
        if (cast != turn_plan.end() && cast->is_array()) {
            std::string on_stage;
            for (const auto& value : *cast) {
                if (!value.is_string()) continue;
                if (!on_stage.empty()) on_stage += ", ";
                on_stage += value.get<std::string>();
            }
            if (!on_stage.empty())
                os << "On this stage: " << on_stage << "\n\n";
        }
    }
    os << "This take:\n" << prose;
    const auto speech = turn_plan.is_object() ? turn_plan.find("speech_turns")
                                              : turn_plan.end();
    if (speech != turn_plan.end() && speech->is_array()) {
        bool any_spoken = false;
        for (const auto& cue : *speech) {
            if (!cue.is_object()) continue;
            const std::string name = str::trim(cue.value("character", ""));
            const std::string line = str::trim(cue.value("line", ""));
            const std::string action = str::trim(cue.value("action", ""));
            if (name.empty() || (line.empty() && action.empty())) continue;
            if (!any_spoken) {
                os << "\n\nSpoken:\n";
                any_spoken = true;
            }
            os << name << ":";
            if (!line.empty()) os << " " << line;
            if (!action.empty()) os << " (" << action << ")";
            os << "\n";
        }
    }
    return os.str();
}

void merge_graph_into_turn_plan(nlohmann::json& turn_plan,
                                const nlohmann::json& graph_plan) {
    if (!turn_plan.is_object()) turn_plan = nlohmann::json::object();
    if (!graph_plan.is_object() || graph_plan.empty()) {
        log_info("narrator") << "graph parse empty -- using []\n"
                             << std::flush;
    }
    auto take_array = [](const nlohmann::json& plan, const char* key) {
        if (plan.is_object()) {
            const auto it = plan.find(key);
            if (it != plan.end() && it->is_array()) return *it;
        }
        return nlohmann::json::array();
    };
    turn_plan["transitions"] = take_array(graph_plan, "transitions");
    turn_plan["new_nodes"] = take_array(graph_plan, "new_nodes");
}

}  // namespace

std::string call_narrator(
    TurnServices& services, const SceneData& scene,
    const std::string& instructions, const std::string& turn_state,
    const ReadToolCallback& read_tool) {
    std::string raw;
    if (services.narrator) {
        raw = services.narrator(
            scene.scene_id, instructions, turn_state, read_tool);
    } else {
        raw = services.llm(instructions + "\n\n" + turn_state);
    }
    return sanitize_utf8(raw);
}

void register_new_characters(
    World& world, SceneData& scene, int turn, const nlohmann::json& plan) {
    if (!plan.contains("new_characters") || !plan["new_characters"].is_array()) {
        return;
    }

    for (const auto& ch_j : plan["new_characters"]) {
        Character ch;
        ch.name = ch_j.value("name", "");
        ch.description = ch_j.value("description", "");
        ch.dialogue_instructions = ch_j.value("dialogue_instructions", "");
        ch.role = ch_j.value("role", "minor_npc");
        ch.created_at = turn;

        if (!ch.name.empty() && !world.find_in_scene(scene.scene_id, ch.name)) {
            log() << "  [enter] " << ch.name << " enters the stage\n";
            world.enter_character(scene.scene_id, std::move(ch));
        }
    }
}

NarratorTurnResult run_narrator_with_retry(
    World& world, TurnServices& services, SceneData& scene, int turn,
    const std::string& instructions, const std::string& turn_state,
    const ReadToolCallback& read_tool) {
    log_info("narrator") << "turn LLM scene=" << scene.scene_id << "\n"
                         << std::flush;

    NarratorTurnResult result;
    std::vector<Rejection> rejections;

    for (int attempt = 0; attempt < kMaxNarratorAttempts; ++attempt) {
        const std::string attempt_state = attempt == 0
            ? turn_state
            : build_revision_turn_state(turn_state, rejections);

        auto raw = call_narrator(
            services, scene, instructions, attempt_state, read_tool);
        log_info("narrator") << "turn response=" << raw.size()
                             << " chars\n" << std::flush;
        if (verbose_logging_enabled()) {
            log_debug("narrator") << "--- NARRATOR turn RESPONSE ---\n" << raw
                                  << "\n--- END NARRATOR turn RESPONSE ---\n"
                                  << std::flush;
        }
        std::tie(result.prose, result.plan) =
            split_merged_response(std::move(raw));

        rejections = validate_active_cast(result.plan, world.characters());
        auto speech = validate_speech_turns(result.plan, world.characters());
        rejections.insert(rejections.end(), speech.begin(), speech.end());
        if (rejections.empty()) break;
        log_info("narrator") << "retry turn attempt " << attempt + 1 << "/"
                             << kMaxNarratorAttempts << ": "
                             << rejections.size() << " issue(s)\n";
        for (const auto& r : rejections)
            log_info("narrator") << "  - " << r.fact << " -- " << r.reason << "\n";
        log() << std::flush;
    }

    register_new_characters(world, scene, turn, result.plan);
    return result;
}

GraphPlanResult extract_graph_observations(
    World& world, WorldGraph& observations, TurnServices& services,
    const SceneData& scene, int turn,
    NarratorTurnResult& narrator, const ReadToolCallback& read_tool) {
    log_info("narrator") << "graph LLM scene=" << scene.scene_id << "\n"
                         << std::flush;
    auto raw_graph = call_narrator(
        services, scene, build_narrator_graph_instructions(),
        build_graph_turn_state(narrator.prose, narrator.plan),
        read_tool);
    log_info("narrator") << "graph response=" << raw_graph.size()
                         << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log_debug("narrator") << "--- NARRATOR graph RESPONSE ---\n" << raw_graph
                              << "\n--- END NARRATOR graph RESPONSE ---\n"
                              << std::flush;
    }
    auto [ignored_prose, graph_plan] =
        split_merged_response(std::move(raw_graph));
    (void)ignored_prose;
    merge_graph_into_turn_plan(narrator.plan, graph_plan);

    (void)world;
    GraphPlanResult output = apply_graph_plan(
        observations, turn, narrator.plan);
    return output;
}

void apply_narrator_cast(
    World& world, SceneData& scene, const NarratorTurnResult& result) {
    if (!result.plan.contains("active_cast") || !result.plan["active_cast"].is_array()) {
        log() << "  [cast] active_cast missing -- keeping current cast\n";
        return;
    }

    std::unordered_set<std::string> resolved_keys;
    std::vector<std::string> resolved_names;
    auto add_resolved = [&](const Character* character) {
        if (!character || character->dead) return;
        const std::string key = str::to_lower(character->name);
        if (resolved_keys.insert(key).second) {
            resolved_names.push_back(character->name);
        }
    };

    for (const auto& elem : result.plan["active_cast"]) {
        if (!elem.is_string()) continue;
        add_resolved(resolve_cast_name(elem.get<std::string>(), world.characters()));
    }

    int speech_count = 0;
    const auto speech = result.plan.find("speech_turns");
    if (speech != result.plan.end() && speech->is_array()) {
        for (const auto& el : *speech) {
            if (!el.is_object()) continue;
            const auto name = el.value("character", "");
            if (name.empty()) continue;
            ++speech_count;
            add_resolved(resolve_cast_name(name, world.characters()));
        }
    }

    if (resolved_names.empty() && speech_count > 0) {
        log() << "  [cast] active_cast resolved empty but " << speech_count
              << " speech cue(s) -- keeping current cast\n";
        return;
    }

    world.add_scene_characters(scene.scene_id, resolved_names);
}

}  // namespace rhapsode
