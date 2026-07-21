#pragma once

#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "rhapsode/llm_callback.h"
#include "rhapsode/scene_message.h"

namespace rhapsode {

struct Snippet {
    std::string text;
    int turn_start = 0;
    int turn_end = 0;
    double timestamp = 0.0;
    bool promoted = false;
    int source_mip = -1;
    int merged_count = 1;
};

struct MipLevel {
    std::vector<Snippet> snippets;
    int max_snippets = 10;
};

struct DownsamplingState {
    std::vector<MipLevel> levels = {
        MipLevel{{}, 10}, MipLevel{{}, 5}, MipLevel{{}, 3}};
    int summarized_up_to = 0;
};

void process_text_downsampling(DownsamplingState& state,
                               const std::vector<SceneMessage>& messages,
                               const LLMCallback& llm_callback,
                               int verbatim_tail = 6);
std::string render_text_downsampling(const DownsamplingState& state);
nlohmann::json downsampling_to_json(const DownsamplingState& state);
DownsamplingState downsampling_from_json(const nlohmann::json& value);

}  // namespace rhapsode
