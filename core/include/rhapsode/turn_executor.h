#pragma once

#include <future>
#include <memory>
#include <stdexcept>
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

struct SceneTurnResult {
    std::string scene_id;
    int completed_turn = 0;
    std::vector<SceneMessage> outputs;
    DirectorOutput director;
    WeaveResult weave;
    std::vector<ExpiryOp> expiry;
};

enum class LoopState {
    Idle,
    ProcessingInput,
    BuildingPrompt,
    RunningLLM,
    AppendingResult
};

using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

// Executes one complete turn against Story-owned state. SceneLoop owns its graph
// services, borrows World for its lifetime, and retains no SceneData between calls.
class SceneLoop {
public:
    SceneLoop(World& world, Director& director, Weaver& weaver);

    SceneTurnResult run_player_turn(SceneData& scene, const std::string& text);
    SceneTurnResult run_autonomous_turn(SceneData& scene, const std::string& focus);
    LoopState state() const { return state_; }

    void set_llm_callback(LLMCallback cb) { llm_cb_ = std::move(cb); }
    void set_narrator_llm_callback(NarratorLLMCallback cb) {
        narrator_llm_cb_ = std::move(cb);
    }
    void set_turn_complete_callback(TurnCompleteCallback cb) {
        turn_complete_cb_ = std::move(cb);
    }
    void set_downsampler_callback(LLMCallback cb) { downsampler_cb_ = std::move(cb); }
    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool value) { resuming_ = value; }

private:
    enum class OutputBucket { Story, Dialogue };

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

    struct BackgroundResult {
        WeaveResult weave;
        std::vector<ExpiryOp> expiry;
    };

    struct TurnWork {
        std::vector<SceneMessage> outputs;
        DirectorOutput director;
    };

    SceneTurnResult run_turn(SceneData& scene, const std::string& text, bool autonomous);
    void submit_message(SceneData& scene, const std::string& text, bool autonomous,
                        TurnWork& work);
    std::future<BackgroundResult> advance(SceneData& scene, TurnWork& work);
    NarratorPrompt build_turn_prompt(SceneData& scene, int turn);
    std::string call_narrator(const SceneData& scene,
                              const std::string& instructions,
                              const std::string& turn_state) const;
    NarratorTurnResult run_narrator_with_retry(SceneData& scene, int turn,
                                                const NarratorPrompt& prompt,
                                                DirectorOutput& director_output);
    void register_new_characters(SceneData& scene, int turn,
                                 const nlohmann::json& plan);
    void apply_narrator_cast(SceneData& scene, const NarratorTurnResult& result);
    void emit_dialogue(SceneData& scene, int turn,
                       const std::vector<SpeechCue>& cues, TurnWork& work);
    void emit_output(SceneData& scene, SceneMessage message,
                     OutputBucket bucket, TurnWork& work);
    void confirm_deaths(const std::vector<DeathCandidate>& candidates,
                        const std::string& narration);
    std::future<BackgroundResult> dispatch_background(SceneData& scene, int turn,
                                                       const DirectorOutput& director_output);
    BackgroundResult finish_background(std::future<BackgroundResult>& future) noexcept;
    World& world_;
    Director& director_;
    Weaver& weaver_;

    LoopState state_ = LoopState::Idle;
    LLMCallback llm_cb_;
    NarratorLLMCallback narrator_llm_cb_;
    TurnCompleteCallback turn_complete_cb_;
    LLMCallback downsampler_cb_;

    size_t window_size_ = 8;
    size_t resume_window_size_ = 12;
    bool resuming_ = false;
};

}  // namespace rhapsode
