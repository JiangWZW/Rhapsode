#pragma once
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
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

class TextDownsampler {
public:
    TextDownsampler();

    void set_llm_callback(LLMCallback cb);
    bool has_llm_callback() const { return static_cast<bool>(llm_cb_); }

    void process_turn(const std::vector<SceneMessage>& messages, int verbatim_tail = 6);
    std::string render() const;
    int summarized_up_to() const { return summarized_up_to_; }

    nlohmann::json to_json() const;
    static TextDownsampler from_json(const nlohmann::json& j);

private:
    LLMCallback llm_cb_;
    std::vector<MipLevel> levels_;
    int summarized_up_to_ = 0;

    std::string summarize(const std::string& passage, int level);
    std::string render_prior_context(int target_level) const;
    void cascade(int source_level);
};

} // namespace rhapsode
