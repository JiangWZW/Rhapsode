#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"
#include <iostream>

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

    auto recent = scene_->history.snapshot(6);
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

    std::cerr << "\n====== Turn " << scene_->turn_index << " ======\n";

    state_ = LoopState::BuildingPrompt;

    std::cerr << "[1/5] Director tick...\n" << std::flush;
    last_director_out_ = {};
    if (director_)
        last_director_out_ = director_->tick(scene_->turn_index, build_scene_context());
    else
        std::cerr << "  (no director attached)\n";

    std::cerr << "[2/5] Building prompt...\n" << std::flush;
    size_t win = resuming_ ? resume_window_size_ : window_size_;
    auto history_snap = scene_->history.snapshot(win);
    resuming_ = false;
    std::string prompt = prompt_cb_(history_snap, *scene_, last_director_out_);
    ++scene_->turn_index;

    {
        std::string sep(60, '-');
        std::cerr << "\n  " << sep << "\n  --- Narrator prompt ---\n";
        std::string::size_type start = 0;
        while (start < prompt.size()) {
            auto end = prompt.find('\n', start);
            if (end == std::string::npos) end = prompt.size();
            std::cerr << "  | " << prompt.substr(start, end - start) << "\n";
            start = end + 1;
        }
        std::cerr << "  " << sep << "\n" << std::flush;
    }

    state_ = LoopState::RunningLLM;

    std::cerr << "[3/5] Calling narrative LLM...\n" << std::flush;
    std::string response = llm_cb_(prompt);
    std::cerr << "  response length: " << response.size() << " chars\n";

    state_ = LoopState::AppendingResult;

    std::cerr << "[4/5] Appending to history...\n" << std::flush;
    SceneMessage assistant_msg;
    assistant_msg.role    = Role::Assistant;
    assistant_msg.content = std::move(response);
    scene_->history.append(assistant_msg);

    std::cerr << "[5/5] Turn complete callback...\n" << std::flush;
    if (turn_complete_cb_)
        turn_complete_cb_(assistant_msg);

    state_ = LoopState::WaitingForInput;
    std::cerr << "====== Turn " << (scene_->turn_index - 1) << " done ======\n" << std::flush;
}

void SceneLoop::set_history_window(size_t normal, size_t resume) {
    window_size_ = normal;
    resume_window_size_ = resume;
}

} // namespace rhapsode
