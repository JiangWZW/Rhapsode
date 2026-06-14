#pragma once
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <stdexcept>
#include "rhapsode/scene_message.h"
#include "rhapsode/director.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

class Scene;
struct DeathCandidate;

enum class LoopState {
    Idle,
    WaitingForInput,
    ProcessingInput,
    Weaving,
    BuildingPrompt,
    RunningLLM,
    AppendingResult
};

using PromptCallback =
    std::function<std::pair<std::string, std::string>(
                              const std::vector<SceneMessage>&,
                              const Scene&,
                              const DirectorOutput&,
                              const std::string& director_focus_text,
                              const std::string& inner_states)>;
using LLMCallback          = std::function<std::string(const std::string& prompt)>;
using NarratorLLMCallback  = std::function<std::string(const std::string& system_msg,
                                                        const std::string& user_msg)>;
using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

class SceneLoop {
public:
    void load_scene(Scene& scene);
    void submit_input(const std::string& text);
    LoopState state() const;

    void set_prompt_callback(PromptCallback cb);
    void set_llm_callback(LLMCallback cb);
    /// Two-part (system, user) callback for the narrator LLM.
    /// Falls back to single-string llm_cb_ with concatenation if unset.
    void set_narrator_llm_callback(NarratorLLMCallback cb);
    void set_turn_complete_callback(TurnCompleteCallback cb);
    void set_director(Director* director);
    const DirectorOutput& last_director_output() const;

    void set_weaver(Weaver* weaver);
    const WeaveResult& last_weave_result() const;

    /** Messages produced by the previous turn's submit_input (narrator + character lines).
     *  Cleared on consume -- typical pattern: call submit_input(), then drain(). */
    std::vector<SceneMessage> take_last_turn_outputs();

    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool v) { resuming_ = v; }

    void set_saves_dir(const std::string& dir);

    /// Block until the background thread finishes.  Must be called before
    /// dropping a SceneLoop reference (undo, error recovery, disconnect).
    void join_background();

    /// Return expiry ops produced by the previous turn's background drain.
    std::vector<ExpiryOp> take_completed_expiry_ops();

private:
    void advance();
    void emit_output(SceneMessage msg);
    std::string build_scene_context() const;
    void dispatch_background();
    void confirm_deaths(const std::vector<DeathCandidate>& candidates,
                        const std::string& narration);

    LoopState state_ = LoopState::Idle;
    Scene* scene_ = nullptr;
    PromptCallback prompt_cb_;
    LLMCallback llm_cb_;
    NarratorLLMCallback narrator_llm_cb_;
    TurnCompleteCallback turn_complete_cb_;
    Director* director_ = nullptr;
    DirectorOutput last_director_out_;
    Weaver* weaver_ = nullptr;
    WeaveResult last_weave_result_;
    std::vector<SceneMessage> last_turn_outputs_;

    size_t window_size_ = 8;
    size_t resume_window_size_ = 12;
    bool resuming_ = false;

    // Background work (single thread: weave -> expiry -> reflections)
    std::future<void> bg_future_;
    std::function<void()> bg_stop_;
    std::string saves_dir_;
    WeaveResult bg_weave_result_;
    std::vector<ExpiryOp> bg_expiry_ops_;
    std::vector<ExpiryOp> completed_expiry_ops_;
};

} // namespace rhapsode
