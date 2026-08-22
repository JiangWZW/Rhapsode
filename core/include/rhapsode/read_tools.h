#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "rhapsode/llm_callback.h"

namespace rhapsode {

class World;
class WorldGraph;
struct SceneData;

/// Non-owning read view. Its pointers must outlive the corresponding lease.
struct ReadToolContext {
    const World* world = nullptr;
    const WorldGraph* observations = nullptr;
    const SceneData* scene = nullptr;
    std::string scene_id;
    std::string scene_summaries_json;
    std::unordered_map<std::string, const SceneData*> scenes_by_id;
};

ReadToolContext make_read_tool_context(
    const World& world,
    const WorldGraph& observations,
    std::string scene_id,
    std::string scene_summaries_json,
    std::unordered_map<std::string, const SceneData*> scenes_by_id);

/// Produces a call-scoped callback over a ReadToolContext. Callback copies fail
/// after the lease closes; keep_alive optionally owns snapshot backing storage.
class ReadToolLease {
public:
    explicit ReadToolLease(
        ReadToolContext context,
        std::shared_ptr<const void> keep_alive = {});
    ~ReadToolLease();

    ReadToolLease(const ReadToolLease&) = delete;
    ReadToolLease& operator=(const ReadToolLease&) = delete;

    const ReadToolCallback& callback() const { return callback_; }
    void close() noexcept;

private:
    struct State;
    std::shared_ptr<State> state_;
    ReadToolCallback callback_;
};

std::string dispatch_read_tool(const ReadToolContext& context,
                               const std::string& name,
                               const std::string& args_json);

}  // namespace rhapsode
