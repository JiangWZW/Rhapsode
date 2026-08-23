#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace rhapsode {

using LLMCallback = std::function<std::string(const std::string&)>;

using ReadToolCallback = std::function<std::string(
    const std::string& name, const std::string& args_json)>;

using NarratorLLMCallback = std::function<std::string(
    const std::string& scene_id,
    const std::string& instructions,
    const std::string& turn_state,
    const ReadToolCallback& read_tool)>;


// Async sessions
struct PromptJob {
    std::size_t handle;
    std::string prompt;
    int staging_buf_id;
};






}  // namespace rhapsode
