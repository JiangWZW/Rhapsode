#include "rhapsode/turn_pipeline.h"

#include "turn_pipeline_internal.h"

#include "rhapsode/character.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/str_util.h"
#include "rhapsode/world.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
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

    if (cast_lower == character_lower) {
        return true;
    }

    // Prefer word-level containment over arbitrary substring matching. This keeps
    // "Al" from matching "Alice" while still accepting "Captain Reed" vs "Reed".
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

bool is_player_speech_name(const std::string& name,
                           const std::vector<Character>& characters) {
    if (str::to_lower(name) == "player") return true;
    for (const auto& ch : characters) {
        if (ch.is_player && resolve_cast_name(name, characters) == &ch) return true;
    }
    return false;
}

bool is_npc_speech_cue(const std::string& name,
                       const std::vector<Character>& characters) {
    if (is_player_speech_name(name, characters)) return false;
    const Character* ch = resolve_cast_name(name, characters);
    return ch && !ch->dead;
}

int count_speakable_npcs_in_cast(const nlohmann::json& plan,
                                 const std::vector<Character>& characters) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) return 0;
    int count = 0;
    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        const Character* ch = resolve_cast_name(elem.get<std::string>(), characters);
        if (ch && !ch->dead) ++count;
    }
    return count;
}

std::vector<Rejection> validate_speech_turns(const nlohmann::json& plan,
                                             const std::vector<Character>& characters) {
    std::vector<Rejection> rejections;
    if (!plan.contains("speech_turns") || !plan["speech_turns"].is_array())
        return rejections;

    const auto& turns = plan["speech_turns"];
    int npc_cues = 0;
    for (const auto& el : turns) {
        if (!el.is_object()) continue;
        const auto name = el.value("character", "");
        if (name.empty()) continue;
        if (is_player_speech_name(name, characters)) {
            rejections.push_back({
                "speech_turns includes \"" + name + "\"",
                "Player must not appear in speech_turns; the user message is the "
                "player's speech -- give responding NPCs their own speech_turn entries"
            });
            continue;
        }
        if (is_npc_speech_cue(name, characters)) ++npc_cues;
    }

    if (turns.empty()) return rejections;

    if (count_speakable_npcs_in_cast(plan, characters) > 0 && npc_cues == 0) {
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

std::vector<Rejection> validate_turn_plan(
    const nlohmann::json& plan,
    const std::vector<Character>& characters) {
    auto rejections = validate_active_cast(plan, characters);
    auto speech = validate_speech_turns(plan, characters);
    rejections.insert(rejections.end(), speech.begin(), speech.end());
    return rejections;
}

void log_narrator_phase(std::string_view phase, const std::string& raw) {
    log_info("narrator") << "phase=" << phase << " response=" << raw.size()
                         << " chars\n" << std::flush;
    if (!verbose_logging_enabled()) return;
    log_debug("narrator") << "--- NARRATOR " << phase << " RESPONSE ---\n" << raw
                          << "\n--- END NARRATOR " << phase << " RESPONSE ---\n"
                          << std::flush;
}

void log_rejections(std::string_view label, int attempt,
                    const std::vector<Rejection>& rejections) {
    log_info("narrator") << "retry " << label << " attempt " << attempt << "/"
                         << kMaxNarratorAttempts << ": " << rejections.size()
                         << " issue(s)\n";
    for (const auto& r : rejections)
        log_info("narrator") << "  - " << r.fact << " -- " << r.reason << "\n";
    log() << std::flush;
}

nlohmann::json json_array_or_empty(const nlohmann::json& plan,
                                   const char* key) {
    if (plan.is_object()) {
        const auto it = plan.find(key);
        if (it != plan.end() && it->is_array()) return *it;
    }
    return nlohmann::json::array();
}

std::string join_string_array(const nlohmann::json& values) {
    std::string result;
    for (const auto& value : values) {
        if (!value.is_string()) continue;
        if (!result.empty()) result += ", ";
        result += value.get<std::string>();
    }
    return result;
}

std::string build_graph_turn_state(const std::string& prose,
                                   const nlohmann::json& turn_plan) {
    std::ostringstream os;
    const std::string on_stage =
        join_string_array(json_array_or_empty(turn_plan, "active_cast"));
    if (!on_stage.empty())
        os << "On this stage: " << on_stage << "\n\n";
    os << "This take:\n" << prose;
    const auto speech = json_array_or_empty(turn_plan, "speech_turns");
    bool any_spoken = false;
    for (const auto& cue : speech) {
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
    return os.str();
}

void merge_graph_into_turn_plan(nlohmann::json& turn_plan,
                                const nlohmann::json& graph_plan) {
    if (!turn_plan.is_object()) turn_plan = nlohmann::json::object();
    if (!graph_plan.is_object() || graph_plan.empty()) {
        log_info("narrator") << "phase=graph parse empty -- using []\n"
                             << std::flush;
    }
    turn_plan["transitions"] = json_array_or_empty(graph_plan, "transitions");
    turn_plan["new_nodes"] = json_array_or_empty(graph_plan, "new_nodes");
}

class LegacyShadowBuilder {
public:
    LegacyShadowBuilder(const World& world, const SceneData& scene, int turn,
                        const nlohmann::json& plan,
                        const std::vector<SceneMessage>& outputs,
                        std::uint64_t base_state_version,
                        std::uint64_t resulting_state_version)
        : world_(world),
          scene_(scene),
          turn_(turn),
          plan_(plan),
          outputs_(outputs),
          base_state_version_(base_state_version),
          resulting_state_version_(resulting_state_version) {
        shadow_.decision.source = shadow_.source;
        shadow_.decision.base_state_version = base_state_version_;
        shadow_.decision.decision_id = versioned_id("legacy:decision");
    }

    LegacyTurnShadow build() {
        add_actor_proposals();
        add_cast_operations();
        add_new_character_operations();
        copy_observation_graph_plan();
        compare_emitted_dialogue();
        return std::move(shadow_);
    }

private:
    std::string versioned_id(const std::string& suffix) const {
        return scene_.scene_id + ":v" +
            std::to_string(resulting_state_version_) + ":" + suffix;
    }

    static std::string indexed_path(const char* field, std::size_t index) {
        return std::string(field) + "[" + std::to_string(index) + "]";
    }

    void issue(std::string code, std::string detail) {
        shadow_.issues.push_back({std::move(code), std::move(detail)});
    }

    const nlohmann::json* array_field(const char* field,
                                      const char* malformed_code) {
        const auto value = plan_.find(field);
        if (value == plan_.end()) return nullptr;
        if (value->is_array()) return &*value;
        issue(malformed_code, std::string(field) + " is not an array");
        return nullptr;
    }

    std::optional<std::string> required_string(
        const nlohmann::json& value, const char* field,
        const std::string& path, const char* malformed_code) {
        const auto text = value.find(field);
        if (text == value.end() || !text->is_string() ||
            str::trim(text->get<std::string>()).empty()) {
            issue(malformed_code,
                  path + " has no string " + std::string(field));
            return std::nullopt;
        }
        return text->get<std::string>();
    }

    bool read_optional_text(const nlohmann::json& value, const char* field,
                            const std::string& path,
                            std::string& destination) {
        const auto text = value.find(field);
        if (text == value.end() || text->is_null()) return true;
        if (!text->is_string()) {
            issue("malformed_actor_proposal",
                  path + "." + field + " is not a string");
            return false;
        }
        destination = str::trim(text->get<std::string>());
        return true;
    }

    const Character* accepted_actor(const std::string& name) const {
        const Character* character = resolve_cast_name(name, world_.characters());
        if (!character || character->is_player || character->dead ||
            !character->in_scene(scene_.scene_id)) {
            return nullptr;
        }
        return character;
    }

    MechanicalOperation operation(MechanicalOperationKind kind,
                                  const Character& character,
                                  nlohmann::json arguments) const {
        MechanicalOperation result;
        result.kind = kind;
        result.scene_id = scene_.scene_id;
        result.character_id = character.name;
        result.arguments = std::move(arguments);
        return result;
    }

    void add_actor_proposals() {
        const nlohmann::json* speech =
            array_field("speech_turns", "malformed_speech_turns");
        if (!speech) return;

        for (std::size_t index = 0; index < speech->size(); ++index) {
            const auto& value = (*speech)[index];
            const std::string path = indexed_path("speech_turns", index);
            if (!value.is_object()) {
                issue("malformed_actor_proposal", path + " is not an object");
                continue;
            }
            const auto requested = required_string(
                value, "character", path, "malformed_actor_proposal");
            if (!requested) continue;
            const Character* character = accepted_actor(*requested);
            if (!character) {
                issue("invalid_actor_reference",
                      path + " does not name a living NPC in this scene");
                continue;
            }

            ActorProposal proposal;
            if (!read_optional_text(value, "action", path, proposal.action) ||
                !read_optional_text(
                    value, "line", path, proposal.exact_dialogue)) {
                continue;
            }
            if (proposal.action.empty() && proposal.exact_dialogue.empty()) {
                issue("empty_actor_proposal",
                      path + " has neither action nor dialogue");
                continue;
            }

            proposal.proposal_id = versioned_id(
                "legacy:proposal:" + std::to_string(shadow_.proposals.size()));
            proposal.character_id = character->name;
            proposal.base_state_version = base_state_version_;
            proposal.source = shadow_.source;
            // Legacy plans contain neither raw evidence references nor a
            // CharacterCore version, so both fields intentionally stay empty.
            shadow_.decision.accepted_proposal_ids.push_back(
                proposal.proposal_id);
            shadow_.proposals.push_back(std::move(proposal));
        }
    }

    void add_cast_operations() {
        const nlohmann::json* active_cast =
            array_field("active_cast", "malformed_active_cast");
        if (!active_cast) return;

        std::unordered_set<std::string> seen;
        for (std::size_t index = 0; index < active_cast->size(); ++index) {
            const auto& value = (*active_cast)[index];
            const std::string path = indexed_path("active_cast", index);
            if (!value.is_string()) {
                issue("malformed_active_cast", path + " is not a string");
                continue;
            }
            const Character* character =
                accepted_actor(value.get<std::string>());
            if (!character) {
                issue("invalid_cast_reference",
                      path + " was not accepted into this scene");
                continue;
            }
            if (!seen.insert(str::to_lower(character->name)).second) continue;
            shadow_.decision.mechanical_operations.push_back(operation(
                MechanicalOperationKind::EnsureSceneMember, *character,
                {{"legacy_field", "active_cast"}}));
        }
    }

    void add_new_character_operations() {
        const nlohmann::json* new_characters =
            array_field("new_characters", "malformed_new_characters");
        if (!new_characters) return;

        std::vector<MechanicalOperation> create_operations;
        for (std::size_t index = 0; index < new_characters->size(); ++index) {
            const auto& value = (*new_characters)[index];
            const std::string path = indexed_path("new_characters", index);
            if (!value.is_object()) {
                issue("malformed_new_character", path + " is not an object");
                continue;
            }
            const auto name = required_string(
                value, "name", path, "malformed_new_character");
            if (!name) continue;
            const Character* character = world_.find_character(*name);
            if (!character || character->created_at != turn_ ||
                !character->in_scene(scene_.scene_id)) {
                issue("unaccepted_new_character",
                      path + " did not create a character this turn");
                continue;
            }
            create_operations.push_back(operation(
                MechanicalOperationKind::CreateCharacter, *character, value));
        }

        shadow_.decision.mechanical_operations.insert(
            shadow_.decision.mechanical_operations.begin(),
            std::make_move_iterator(create_operations.begin()),
            std::make_move_iterator(create_operations.end()));
    }

    void copy_observation_graph_plan() {
        for (const char* field : {"transitions", "new_nodes"}) {
            const nlohmann::json* value = array_field(
                field, "malformed_observation_graph_plan");
            shadow_.observation_graph_plan[field] = value
                ? *value : nlohmann::json::array();
        }
    }

    void compare_emitted_dialogue() {
        std::vector<const SceneMessage*> dialogue;
        for (const auto& output : outputs_) {
            if (output.metadata.value("scene_kind", std::string{}) ==
                "character") {
                dialogue.push_back(&output);
            }
        }
        if (dialogue.size() != shadow_.proposals.size()) {
            issue("legacy_output_difference",
                  "typed proposals and emitted character messages have "
                  "different counts");
        }

        const std::size_t count =
            std::min(dialogue.size(), shadow_.proposals.size());
        for (std::size_t index = 0; index < count; ++index) {
            const ActorProposal& proposal = shadow_.proposals[index];
            std::string expected = proposal.exact_dialogue;
            if (!proposal.action.empty()) {
                expected += (expected.empty() ? "" : " ") +
                    ("(" + proposal.action + ")");
            }
            const SceneMessage& emitted = *dialogue[index];
            if (emitted.content != expected ||
                emitted.metadata.value("speaker", std::string{}) !=
                    proposal.character_id) {
                issue("legacy_output_difference",
                      "proposal " + proposal.proposal_id +
                          " does not reconstruct the emitted message");
            }
        }
    }

    const World& world_;
    const SceneData& scene_;
    int turn_;
    const nlohmann::json& plan_;
    const std::vector<SceneMessage>& outputs_;
    std::uint64_t base_state_version_;
    std::uint64_t resulting_state_version_;
    LegacyTurnShadow shadow_;
};

}  // namespace

NarratorPrompt build_turn_prompt(
    World& world, TurnServices& services, SceneData& scene) {
    log_info("narrator") << "[1/4] building prompt scene=" << scene.scene_id
                         << "\n" << std::flush;

    services.resuming = false;

    NarratorPrompt prompt;
    prompt.instructions = build_narrator_instructions();
    prompt.turn_state = build_narrator_turn_state(
        scene, world, services.storyline_board);
    services.storyline_board.clear();

    ++scene.turn_index;

    log_debug("narrator") << "prompt instructions=" << prompt.instructions.size()
                          << " turn_state=" << prompt.turn_state.size()
                          << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log() << "--- NARRATOR INSTRUCTIONS ---\n" << prompt.instructions << "\n"
              << "--- NARRATOR TURN STATE ---\n" << prompt.turn_state << "\n"
              << "--- END NARRATOR PROMPT ---\n" << std::flush;
    }
    return prompt;
}

std::string call_narrator(
    TurnServices& services, const SceneData& scene,
    const std::string& instructions, const std::string& turn_state,
    const ReadToolCallback& read_tool) {
    const std::string safe_instructions = sanitize_utf8(instructions);
    const std::string safe_turn_state   = sanitize_utf8(turn_state);
    if (services.narrator) {
        return sanitize_utf8(services.narrator(
            scene.scene_id, safe_instructions, safe_turn_state, read_tool));
    }
    return sanitize_utf8(
        services.llm(safe_instructions + "\n\n" + safe_turn_state));
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
    const NarratorPrompt& prompt,
    const ReadToolCallback& read_tool) {
    log_info("narrator") << "[2/4] turn LLM scene=" << scene.scene_id << "\n"
                         << std::flush;

    NarratorTurnResult result;
    std::vector<Rejection> rejections;

    // Phase A — prose / speech / cast (retry on validation only).
    for (int attempt = 0; attempt < kMaxNarratorAttempts; ++attempt) {
        const std::string turn_state = attempt == 0
            ? prompt.turn_state
            : build_revision_turn_state(prompt.turn_state, rejections);

        auto raw = call_narrator(
            services, scene, prompt.instructions, turn_state, read_tool);
        log_narrator_phase("turn", raw);
        std::tie(result.prose, result.plan) =
            split_merged_response(std::move(raw));

        rejections = validate_turn_plan(result.plan, world.characters());
        if (rejections.empty()) break;
        log_rejections("turn", attempt + 1, rejections);
    }

    register_new_characters(world, scene, turn, result.plan);

    const auto speech = result.plan.find("speech_turns");
    if (speech != result.plan.end() && speech->is_array()) {
        for (const auto& el : *speech) {
            if (!el.is_object()) continue;
            auto name = el.value("character", "");
            if (!name.empty()) result.cues.push_back({std::move(name), el});
        }
    }
    return result;
}

GraphPlanResult extract_graph_observations(
    World& world, WorldGraph& observations, TurnServices& services,
    const SceneData& scene, int turn, const NarratorPrompt& prompt,
    NarratorTurnResult& narrator, const ReadToolCallback& read_tool) {
    (void)prompt;
    // Observation extraction runs only after the narrative turn is committed.
    log_info("narrator") << "[2b/4] graph LLM scene=" << scene.scene_id << "\n"
                         << std::flush;
    auto raw_graph = call_narrator(
        services, scene, build_narrator_graph_instructions(),
        build_graph_turn_state(narrator.prose, narrator.plan),
        read_tool);
    log_narrator_phase("graph", raw_graph);
    auto [ignored_prose, graph_plan] =
        split_merged_response(std::move(raw_graph));
    (void)ignored_prose;
    merge_graph_into_turn_plan(narrator.plan, graph_plan);

    log_info("narrator") << "[3/4] recording graph observations scene="
                         << scene.scene_id
                         << "\n" << std::flush;
    GraphPlanResult output = apply_graph_plan(
        observations, turn, narrator.plan);
    world.route_perceptions(scene.scene_id, output.new_nodes, turn);
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
        auto name = elem.get<std::string>();
        add_resolved(resolve_cast_name(name, world.characters()));
    }

    for (const auto& cue : result.cues) {
        add_resolved(resolve_cast_name(cue.character, world.characters()));
    }

    if (resolved_names.empty() && !result.cues.empty()) {
        log() << "  [cast] active_cast resolved empty but " << result.cues.size()
              << " speech cue(s) -- keeping current cast\n";
        return;
    }

    world.add_scene_characters(scene.scene_id, resolved_names);
}

LegacyTurnShadow adapt_legacy_shadow(
    const World& world, const SceneData& scene, int turn,
    const NarratorTurnResult& result,
    const std::vector<SceneMessage>& outputs,
    std::uint64_t base_state_version,
    std::uint64_t resulting_state_version) {
    return LegacyShadowBuilder(
        world, scene, turn, result.plan, outputs, base_state_version,
        resulting_state_version).build();
}

}  // namespace rhapsode
