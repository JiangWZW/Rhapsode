#pragma once

#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/node.h"
#include "rhapsode/scene_message.h"

namespace rhapsode {

struct Character;
class Scene;
struct Rejection;

struct SpeechCue {
    std::string character;
    nlohmann::json direction;

    std::string field(const char* key) const { return direction.value(key, ""); }
};

struct NarratorTurnResult {
    std::string prose;
    nlohmann::json plan;
    std::vector<SpeechCue> cues;
};

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw);

const Character* resolve_cast_name(const std::string& cast_name,
                                   const std::vector<Character>& characters);

std::vector<Rejection> validate_active_cast(const nlohmann::json& plan, const Scene& scene);

std::vector<SpeechCue> extract_speech_cues(const nlohmann::json& plan);

void apply_active_cast(const nlohmann::json& plan,
                       const std::vector<SpeechCue>& cues,
                       Scene& scene);

void route_perception(Scene& scene, const std::vector<Node>& new_nodes, int turn);

SceneMessage make_scene_loop_message(const std::string& kind,
                                     std::string content,
                                     const std::string& speaker = {});

bool is_affirmative_yes_response(const std::string& response);

}  // namespace rhapsode
