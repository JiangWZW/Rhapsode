#include "rhapsode/scene_loop_support.h"

#include "rhapsode/character.h"
#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>
#include <unordered_set>

namespace rhapsode {
namespace {

constexpr char kJsonToken[] = "<<<RHAPSODE_JSON>>>";

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

}  // namespace

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw) {
    // Normalize smart quotes etc. up front so both the marker split and the
    // brace-fallback below operate on parseable text.
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

std::vector<Rejection> validate_active_cast(const nlohmann::json& plan, const Scene& scene) {
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

std::vector<SpeechCue> extract_speech_cues(const nlohmann::json& plan) {
    std::vector<SpeechCue> cues;
    auto it = plan.find("speech_turns");
    if (it == plan.end() || !it->is_array()) {
        return cues;
    }

    for (const auto& el : *it) {
        if (!el.is_object()) {
            continue;
        }
        auto name = el.value("character", "");
        if (!name.empty()) {
            cues.push_back({std::move(name), el});
        }
    }
    return cues;
}

void apply_active_cast(const nlohmann::json& plan,
                       const std::vector<SpeechCue>& cues,
                       Scene& scene) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) {
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

    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) {
            continue;
        }
        auto name = elem.get<std::string>();
        add_resolved(resolve_cast_name(name, scene.world().characters));
    }

    for (const auto& cue : cues) {
        add_resolved(resolve_cast_name(cue.character, scene.world().characters));
    }

    if (resolved_names.empty() && !cues.empty()) {
        log() << "  [cast] active_cast resolved empty but " << cues.size()
              << " speech cue(s) -- keeping current cast\n";
        return;
    }

    scene.ensure_characters_present(resolved_names);
}

void route_perception(Scene& scene, const std::vector<Node>& new_nodes, int turn) {
    scene.world().route_perceptions(scene.scene_id, new_nodes, turn);
}

SceneMessage make_scene_loop_message(const std::string& kind,
                                     std::string content,
                                     const std::string& speaker) {
    SceneMessage msg;
    msg.role = Role::Assistant;
    msg.content = std::move(content);
    msg.metadata = {{"scene_kind", kind}};
    if (!speaker.empty()) {
        msg.metadata["speaker"] = speaker;
    }
    return msg;
}

bool is_affirmative_yes_response(const std::string& response) {
    const std::string lower = str::to_lower(str::trim(response));
    if (lower.size() < 3 || lower.compare(0, 3, "yes") != 0) {
        return false;
    }
    return lower.size() == 3 || str::is_word_boundary(lower, 3);
}

}  // namespace rhapsode
