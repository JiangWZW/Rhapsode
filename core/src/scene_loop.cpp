#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"

namespace rhapsode {

void SceneLoop::load_scene(Scene& scene) {
    scene_ = &scene;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::submit_input(const std::string& text) {
    if (state_ != LoopState::WaitingForInput)
        throw std::runtime_error("Cannot submit input: loop is not waiting for input");

    state_ = LoopState::ProcessingInput;

    SceneMessage user_msg;
    user_msg.role    = Role::User;
    user_msg.content = text;
    scene_->history.append(std::move(user_msg));

    advance();
}

LoopState SceneLoop::state() const { return state_; }

void SceneLoop::set_prompt_callback(PromptCallback cb)       { prompt_cb_        = std::move(cb); }
void SceneLoop::set_llm_callback(LLMCallback cb)             { llm_cb_           = std::move(cb); }
void SceneLoop::set_turn_complete_callback(TurnCompleteCallback cb) { turn_complete_cb_ = std::move(cb); }
void SceneLoop::set_director(Director* director)              { director_         = director; }
const DirectorOutput& SceneLoop::last_director_output() const { return last_director_out_; }

std::string SceneLoop::build_scene_context() const {
    std::string ctx = scene_->title;

    auto recent = scene_->history.snapshot(3);
    for (const auto& msg : recent) {
        ctx += "\n";
        ctx += (msg.role == Role::User ? "user: " : "assistant: ");
        ctx += msg.content;
    }

    return ctx;
}

void SceneLoop::advance() {
    if (!prompt_cb_)
        throw std::runtime_error("No prompt callback registered");
    if (!llm_cb_)
        throw std::runtime_error("No LLM callback registered");

    state_ = LoopState::BuildingPrompt;

    last_director_out_ = {};
    if (director_)
        last_director_out_ = director_->tick(turn_index_, build_scene_context());

    auto history_snap = scene_->history.snapshot();
    std::string prompt = prompt_cb_(history_snap, *scene_, last_director_out_);
    ++turn_index_;

    state_ = LoopState::RunningLLM;

    std::string response = llm_cb_(prompt);

    state_ = LoopState::AppendingResult;

    SceneMessage assistant_msg;
    assistant_msg.role    = Role::Assistant;
    assistant_msg.content = std::move(response);
    scene_->history.append(assistant_msg);

    if (turn_complete_cb_)
        turn_complete_cb_(assistant_msg);

    state_ = LoopState::WaitingForInput;
}

} // namespace rhapsode
