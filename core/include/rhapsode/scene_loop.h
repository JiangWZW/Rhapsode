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

using PromptCallback = std::function<std::string(const std::vector<SceneMessage>&, const Scene&, const DirectorOutput&)>;
using LLMCallback = std::function<std::string(const std::string& prompt)>;
using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

class SceneLoop {
public:
    void load_scene(Scene& scene);
    void submit_input(const std::string& text);
    LoopState state() const;

    void set_prompt_callback(PromptCallback cb);
    void set_llm_callback(LLMCallback cb);
    void set_turn_complete_callback(TurnCompleteCallback cb);
    void set_director(Director* director);
    const DirectorOutput& last_director_output() const;

private:
    void advance();
    std::string build_scene_context() const;

    LoopState state_ = LoopState::Idle;
    Scene* scene_ = nullptr;
    PromptCallback prompt_cb_;
    LLMCallback llm_cb_;
    TurnCompleteCallback turn_complete_cb_;
    Director* director_ = nullptr;
    DirectorOutput last_director_out_;
    int turn_index_ = 0;
};

} // namespace rhapsode
