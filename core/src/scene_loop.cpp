#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"

namespace rhapsode {

void SceneLoop::load_scene(Scene& scene) 
{
    scene_ = &scene;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::submit_input(const std::string& text) {
    if (state_ != LoopState::WaitingForInput) {
        throw std::runtime_error("Cannot submit input: loop is not waiting for input");
    }

    state_ = LoopState::ProcessingInput;

    SceneMessage user_msg;
    user_msg.role = Role::User;
    user_msg.content = text;
    scene_->history.append(std::move(user_msg));

    advance();
}

LoopState SceneLoop::state() const {
    return state_;
}

void SceneLoop::set_prompt_callback(PromptCallback cb) {
    prompt_cb_ = std::move(cb);
}

void SceneLoop::set_llm_callback(LLMCallback cb) {
    llm_cb_ = std::move(cb);
}

void SceneLoop::set_turn_complete_callback(TurnCompleteCallback cb) {
    turn_complete_cb_ = std::move(cb);
}

void SceneLoop::advance()
{
    // ProcessingInput -> BuildingPrompt
    state_ = LoopState::BuildingPrompt;

    if (!prompt_cb_) {
        throw std::runtime_error("No prompt callback registered");
    }
    auto history_snap = scene_->history.snapshot();
    std::string prompt = prompt_cb_(history_snap, *scene_);

    // BuildingPrompt -> RunningLLM
    state_ = LoopState::RunningLLM;

    if (!llm_cb_) {
        throw std::runtime_error("No LLM callback registered");
    }
    std::string response = llm_cb_(prompt);

    // RunningLLM -> AppendingResult
    state_ = LoopState::AppendingResult;

    SceneMessage assistant_msg;
    assistant_msg.role = Role::Assistant;
    assistant_msg.content = response;
    scene_->history.append(assistant_msg);

    if (turn_complete_cb_) {
        turn_complete_cb_(assistant_msg);
    }

    // AppendingResult -> WaitingForInput
    state_ = LoopState::WaitingForInput;
}

} // namespace rhapsode
