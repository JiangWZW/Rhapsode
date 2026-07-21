#pragma once

#include <string>
#include <vector>

#include "rhapsode/scene_message.h"
#include "rhapsode/text_downsampling.h"

namespace rhapsode {

// State for one storyline. Story owns these records and coordinates them with
// World; a SceneData record deliberately has no World or runtime dependency.
struct SceneData {
    std::string scene_id;
    std::string title;
    std::string system_prompt;

    std::vector<SceneMessage> history;
    std::vector<SceneMessage> dialogue;
    DownsamplingState downsampling;
    int turn_index = 0;

    std::string driving_intention;
    float charge = 0.0f;
    int last_advanced = 0;
};

}  // namespace rhapsode
