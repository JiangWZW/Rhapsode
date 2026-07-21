#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/director.h"
#include "rhapsode/llm_callback.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

class World;
struct DeathCandidate;

struct TurnResult {
    std::string scene_id;
    int completed_turn = 0;
    std::vector<SceneMessage> outputs;
    struct Effects {
        std::vector<Node> created_nodes;
        std::vector<Node> expired_nodes;
    } effects;
};

using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

// Executes one complete turn against Story-owned state. TurnExecutor borrows
// Story's World and graph services and retains no SceneData between calls.
class TurnExecutor {
public:
    TurnExecutor(World& world, Director& director, Weaver& weaver);

    TurnResult run_player_turn(SceneData& scene, const std::string& text,
                               ReadToolCallback read_tool = {});
    TurnResult run_autonomous_turn(SceneData& scene, const std::string& focus,
                                   ReadToolCallback read_tool = {});

    void set_llm_callback(LLMCallback cb) { llm_cb_ = std::move(cb); }
    void set_narrator_llm_callback(NarratorLLMCallback cb) {
        narrator_llm_cb_ = std::move(cb);
    }
    void set_turn_complete_callback(TurnCompleteCallback cb) {
        turn_complete_cb_ = std::move(cb);
    }
    void set_downsampler_callback(LLMCallback cb) { downsampler_cb_ = std::move(cb); }
    void set_reflection_llm_callback(LLMCallback cb) {
        reflection_llm_cb_ = std::move(cb);
    }
    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool value) { resuming_ = value; }

private:
    enum class OutputBucket { Narration, Dialogue };

    struct NarratorPrompt {
        std::string instructions;
        std::string turn_state;
    };

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

    struct PostTurnResult {
        std::vector<Node> expired_nodes;
    };

    struct TurnWork {
        std::vector<SceneMessage> outputs;
        DirectorOutput director_output;
        ReadToolCallback read_tool;
    };

    TurnResult run_turn(SceneData& scene, const std::string& text, bool autonomous,
                        ReadToolCallback read_tool);
    void append_input_message(SceneData& scene, const std::string& text,
                              bool autonomous);
    PostTurnResult execute_turn(SceneData& scene, TurnWork& work);
    NarratorPrompt build_turn_prompt(SceneData& scene);
    std::string call_narrator(const SceneData& scene,
                              const std::string& instructions,
                              const std::string& turn_state,
                              const ReadToolCallback& read_tool) const;
    NarratorTurnResult run_narrator_with_retry(SceneData& scene, int turn,
                                                const NarratorPrompt& prompt,
                                                DirectorOutput& director_output,
                                                const ReadToolCallback& read_tool);
    void register_new_characters(SceneData& scene, int turn,
                                 const nlohmann::json& plan);
    void apply_narrator_cast(SceneData& scene, const NarratorTurnResult& result);
    void emit_dialogue(SceneData& scene, int turn,
                       const std::vector<SpeechCue>& cues, TurnWork& work);
    void emit_output(SceneData& scene, SceneMessage message,
                     OutputBucket bucket, TurnWork& work);
    void confirm_deaths(const std::vector<DeathCandidate>& candidates,
                        const std::string& narration);
    PostTurnResult run_post_turn(SceneData& scene, int turn) noexcept;
    World& world_;
    Director& director_;
    Weaver& weaver_;

    bool running_ = false;
    LLMCallback llm_cb_;
    NarratorLLMCallback narrator_llm_cb_;
    TurnCompleteCallback turn_complete_cb_;
    LLMCallback downsampler_cb_;
    LLMCallback reflection_llm_cb_;

    size_t window_size_ = 8;
    size_t resume_window_size_ = 12;
    bool resuming_ = false;
};

}  // namespace rhapsode
