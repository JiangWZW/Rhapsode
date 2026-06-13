#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "rhapsode/world_graph.h"

namespace rhapsode {

struct GraphAnalysis {
    int live_node_count = 0;
    int active_edge_count = 0;
    int orphan_count = 0;
};

GraphAnalysis analyze(const WorldGraph& graph);

struct WeaveOp {
    std::uint64_t from_id = 0;
    std::uint64_t to_id = 0;
    float weight = 1.0f;
    std::string reason;
};

struct WeaveResult {
    std::vector<WeaveOp> connected;
    std::vector<WeaveOp> disconnected;
    std::vector<WeaveOp> reweighted;
    GraphAnalysis analysis;
};

struct ExpiryOp {
    std::uint64_t id = 0;
    std::string reason;
};

using WeaverLLMCallback = std::function<std::string(const std::string&)>;

class Weaver {
public:
    explicit Weaver(WorldGraph& graph);

    void set_llm_callback(WeaverLLMCallback cb);
    void set_local_llm_callback(WeaverLLMCallback cb);
    void set_interval(int turns);
    bool should_weave(int turn_index) const;

    /// Full weave using the cloud LLM (called on should_weave turns).
    WeaveResult weave(int turn_index, const std::string& scene_context = "");
    /// Lightweight weave using the local LLM (called every other turn).
    WeaveResult weave_local(int turn_index, const std::string& scene_context = "");

    // -- Entity-group expiry detector --

    /// Rebuild the expiry queue from the current graph state.
    /// Entity groups matching priority_entities are placed at the front.
    void rebuild_expiry_queue(
        const std::vector<std::string>& priority_entities = {});

    /// Drain the expiry queue, one local-LLM call per entity group.
    /// Loops until queue empty or stop_expiry_drain() is called.
    /// Thread-safe with stop_expiry_drain().
    std::vector<ExpiryOp> drain_expiry_queue(int turn_index);

    /// Signal the drain loop to stop after the current group finishes.
    void stop_expiry_drain();

    bool expiry_queue_empty() const;

private:
    WorldGraph& graph_;
    WeaverLLMCallback llm_cb_;
    WeaverLLMCallback local_llm_cb_;
    int interval_ = 3;

    std::string build_prompt(int turn_index,
                             const std::string& scene_context) const;
    WeaveResult parse_and_apply(const std::string& llm_response,
                                int turn_index);
    WeaveResult weave_impl(int turn_index, const std::string& scene_context,
                           WeaverLLMCallback& cb, const char* label);

    // Selection RNG (mutable: not part of observable Weaver state)
    mutable std::mt19937 rng_{std::random_device{}()};

    // Expiry state
    std::atomic<bool> expiry_stop_{false};
    std::vector<std::vector<std::uint64_t>> expiry_queue_;
    std::vector<ExpiryOp> check_group(std::vector<const Node*> live,
                                      int turn_index);
};

}  // namespace rhapsode
