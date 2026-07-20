#include "rhapsode/scene_loop.h"

#include "rhapsode/character.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/scene.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <array>
#include <cstring>
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
                                            const Scene& scene) {
    std::vector<Rejection> rejections;
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) {
        return rejections;
    }

    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) {
            continue;
        }
        auto name = elem.get<std::string>();
        const Character* ch = resolve_cast_name(name, scene.world().characters);
        if (!ch) {
            log() << "  [cast] active_cast lists unresolved \"" << name << "\" -- ignoring\n";
        } else if (ch->dead) {
            rejections.push_back({"active_cast includes \"" + name + "\"", "character is dead"});
        }
    }
    return rejections;
}

bool is_player_speech_name(const std::string& name, const Scene& scene) {
    if (str::to_lower(name) == "player") return true;
    for (const auto& ch : scene.world().characters) {
        if (ch.is_player && resolve_cast_name(name, scene.world().characters) == &ch) return true;
    }
    return false;
}

bool is_npc_speech_cue(const std::string& name, const Scene& scene) {
    if (is_player_speech_name(name, scene)) return false;
    const Character* ch = resolve_cast_name(name, scene.world().characters);
    return ch && !ch->dead;
}

int count_speakable_npcs_in_cast(const nlohmann::json& plan, const Scene& scene) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) return 0;
    int count = 0;
    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        const Character* ch = resolve_cast_name(elem.get<std::string>(), scene.world().characters);
        if (ch && !ch->dead) ++count;
    }
    return count;
}

std::vector<Rejection> validate_speech_turns(const nlohmann::json& plan,
                                             const Scene& scene) {
    std::vector<Rejection> rejections;
    if (!plan.contains("speech_turns") || !plan["speech_turns"].is_array())
        return rejections;

    const auto& turns = plan["speech_turns"];
    int npc_cues = 0;
    for (const auto& el : turns) {
        if (!el.is_object()) continue;
        const auto name = el.value("character", "");
        if (name.empty()) continue;
        if (is_player_speech_name(name, scene)) {
            rejections.push_back({
                "speech_turns includes \"" + name + "\"",
                "Player must not appear in speech_turns; the user message is the "
                "player's speech -- give responding NPCs their own speech_turn entries"
            });
            continue;
        }
        if (is_npc_speech_cue(name, scene)) ++npc_cues;
    }

    if (turns.empty()) return rejections;

    if (count_speakable_npcs_in_cast(plan, scene) > 0 && npc_cues == 0) {
        rejections.push_back({
            "speech_turns",
            "NPCs are present in active_cast but no NPC speech_turns were authored "
            "(speech_turns must contain each speaking NPC's line, not the Player's)"
        });
    }
    return rejections;
}

}  // namespace

SceneLoop::NarratorPrompt SceneLoop::build_turn_prompt(int turn) {
    state_ = LoopState::BuildingPrompt;
    log() << "[1/4] Building merged prompt...\n" << std::flush;

    const size_t win = resuming_ ? resume_window_size_ : window_size_;
    const std::vector<SceneMessage> history = scene_->history.snapshot(win);
    resuming_ = false;

    NarratorPrompt prompt;
    prompt.instructions = build_narrator_instructions();
    prompt.turn_state = build_narrator_turn_state(history, *scene_);

    ++scene_->turn_index;

    log() << "  [prompt] instructions=" << prompt.instructions.size()
          << " turn_state=" << prompt.turn_state.size() << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log() << "--- NARRATOR INSTRUCTIONS ---\n" << prompt.instructions << "\n"
              << "--- NARRATOR TURN STATE ---\n" << prompt.turn_state << "\n"
              << "--- END NARRATOR PROMPT ---\n" << std::flush;
    }
    return prompt;
}

std::string SceneLoop::call_narrator(const std::string& instructions,
                                     const std::string& turn_state) const {
    const std::string safe_instructions = sanitize_utf8(instructions);
    const std::string safe_turn_state   = sanitize_utf8(turn_state);
    if (narrator_llm_cb_) {
        return sanitize_utf8(narrator_llm_cb_(
            scene_->scene_id, safe_instructions, safe_turn_state));
    }
    return sanitize_utf8(llm_cb_(safe_instructions + "\n\n" + safe_turn_state));
}

void SceneLoop::rollback_turn_attempt(const World& world_snapshot) {
    scene_->world() = world_snapshot;
}

void SceneLoop::register_new_characters(int turn, const nlohmann::json& plan) {
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

        if (!ch.name.empty() && !scene_->find_on_stage(ch.name)) {
            log() << "  [enter] " << ch.name << " enters the stage\n";
            scene_->enter_character(std::move(ch));
        }
    }
}

SceneLoop::NarratorTurnResult SceneLoop::run_narrator_with_retry(
    int turn, const NarratorPrompt& prompt) {
    state_ = LoopState::RunningLLM;
    log() << "[2/4] Calling narrative LLM...\n" << std::flush;

    scene_->world().clear_pending_ops();
    auto raw_response = call_narrator(prompt.instructions, prompt.turn_state);
    log() << "  [narrator] response=" << raw_response.size() << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log() << "--- NARRATOR RESPONSE ---\n" << raw_response
              << "\n--- END NARRATOR RESPONSE ---\n" << std::flush;
    }

    NarratorTurnResult result;
    std::tie(result.prose, result.plan) = split_merged_response(std::move(raw_response));

    state_ = LoopState::AppendingResult;
    log() << "[3/4] Applying graph...\n" << std::flush;

    std::vector<Rejection> all_rejections;
    const World world_snapshot = scene_->world();

    for (int attempt = 0; attempt < kMaxNarratorAttempts; ++attempt) {
        if (attempt > 0) {
            rollback_turn_attempt(world_snapshot);

            std::string rewrite_turn_state = prompt.turn_state;
            rewrite_turn_state += "\n\n### REVISION REQUIRED\n"
                                  "The following issues were found in your plan:\n";
            for (const auto& r : all_rejections) {
                rewrite_turn_state += "- " + r.fact + " -- " + r.reason + "\n";
            }
            rewrite_turn_state += "\nRewrite your narrative and plan to fix these issues.\n";

            state_ = LoopState::RunningLLM;
            scene_->world().clear_pending_ops();
            auto [new_prose, new_plan] =
                split_merged_response(call_narrator(prompt.instructions, rewrite_turn_state));
            result.prose = std::move(new_prose);
            result.plan = std::move(new_plan);
            state_ = LoopState::AppendingResult;
        }

        last_director_out_ = director_->apply_planned_turn(turn, result.plan);
        register_new_characters(turn, result.plan);

        const auto cast_rejections = validate_active_cast(result.plan, *scene_);
        const auto speech_rejections = validate_speech_turns(result.plan, *scene_);
        all_rejections = last_director_out_.rejections;
        all_rejections.insert(all_rejections.end(), cast_rejections.begin(), cast_rejections.end());
        all_rejections.insert(all_rejections.end(), speech_rejections.begin(), speech_rejections.end());

        if (all_rejections.empty()) {
            break;
        }

        log() << "  [retry] attempt " << (attempt + 1) << "/" << kMaxNarratorAttempts
              << ": " << all_rejections.size() << " issue(s)\n";
        for (const auto& r : all_rejections) {
            log() << "    - " << r.fact << " -- " << r.reason << "\n";
        }
        log() << std::flush;
    }

    auto it = result.plan.find("speech_turns");
    if (it != result.plan.end() && it->is_array()) {
        for (const auto& el : *it) {
            if (!el.is_object()) continue;
            auto name = el.value("character", "");
            if (!name.empty()) result.cues.push_back({std::move(name), el});
        }
    }
    return result;
}

void SceneLoop::apply_narrator_cast(const NarratorTurnResult& result) {
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
        add_resolved(resolve_cast_name(name, scene_->world().characters));
    }

    for (const auto& cue : result.cues) {
        add_resolved(resolve_cast_name(cue.character, scene_->world().characters));
    }

    if (resolved_names.empty() && !result.cues.empty()) {
        log() << "  [cast] active_cast resolved empty but " << result.cues.size()
              << " speech cue(s) -- keeping current cast\n";
        return;
    }

    scene_->ensure_characters_present(resolved_names);
}

}  // namespace rhapsode
