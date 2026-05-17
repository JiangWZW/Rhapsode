#pragma once
#include <functional>
#include <string>
#include <vector>
#include <stdexcept>
#include "rhapsode/scene_message.h"
#include "rhapsode/director.h"

namespace rhapsode {

class Scene;

enum class LoopState {
    Idle,
    WaitingForInput,
    ProcessingInput,
    BuildingPrompt,
    RunningLLM,
    AppendingResult
};

using PromptCallback =
    std::function<std::string(const std::vector<SceneMessage>&,
                              const Scene&,
                              const DirectorOutput&,
                              const std::string& director_focus_json)>;
using LLMCallback          = std::function<std::string(const std::string& prompt)>;
using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

/// Returns one spoken line per cue, in cue order — uses local-model dialogue synthesis.
using CharacterSynthCallback =
    std::function<std::vector<std::string>(const std::vector<std::pair<std::string, std::string>>& cues,
                                           const std::string& narration_prose)>;

class SceneLoop {
public:
    void load_scene(Scene& scene);
    void submit_input(const std::string& text);
    LoopState state() const;

    void set_prompt_callback(PromptCallback cb);
    void set_llm_callback(LLMCallback cb);
    void set_turn_complete_callback(TurnCompleteCallback cb);
    /// Optional: generate NPC dialogue lines from speech cues after narrator prose lands.
    void set_character_synth_callback(CharacterSynthCallback cb);
    void set_director(Director* director);
    const DirectorOutput& last_director_output() const;

    /** Messages produced by the previous turn's submit_input (narrator + character lines).
     *  Cleared on consume — typical pattern: call submit_input(), then drain(). */
    std::vector<SceneMessage> take_last_turn_outputs();

    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool v) { resuming_ = v; }

private:
    void advance();
    std::string build_scene_context() const;

    LoopState state_ = LoopState::Idle;
    Scene* scene_ = nullptr;
    PromptCallback prompt_cb_;
    LLMCallback llm_cb_;
    TurnCompleteCallback turn_complete_cb_;
    CharacterSynthCallback char_synth_cb_;
    Director* director_ = nullptr;
    DirectorOutput last_director_out_;
    std::vector<SceneMessage> last_turn_outputs_;

    size_t window_size_ = 8;
    size_t resume_window_size_ = 12;
    bool resuming_ = false;
};

} // namespace rhapsode
