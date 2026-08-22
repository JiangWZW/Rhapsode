#pragma once

#include <atomic>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
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

class Weaver {
public:
    Weaver() = default;
    Weaver(const Weaver&) = delete;
    Weaver& operator=(const Weaver&) = delete;
    Weaver(Weaver&& other) noexcept;
    Weaver& operator=(Weaver&& other) noexcept;

    void set_llm_callback(LLMCallback cb);
    void set_interval(int turns);
    bool active() const noexcept { return active_; }
    bool should_weave(int turn_index) const;

    /// Graph edge audit using the weaver LLM (called on should_weave turns).
    WeaveResult weave(WorldGraph& graph, int turn_index,
                      const std::string& scene_context = "");

    // -- Entity-group expiry detector --

    /// Rebuild the expiry queue from the current graph state.
    /// Entity groups matching priority_entities are placed at the front.
    void rebuild_expiry_queue(
        const WorldGraph& graph,
        const std::vector<std::string>& priority_entities = {});

    /// Drain the expiry queue, one LLM call per entity group.
    /// Loops until queue empty or stop_expiry_drain() is called.
    /// Thread-safe with stop_expiry_drain().
    std::vector<ExpiryOp> drain_expiry_queue(
        WorldGraph& graph, int turn_index);

    /// Signal the drain loop to stop after the current group finishes.
    void stop_expiry_drain();

    bool expiry_queue_empty() const;

private:
    LLMCallback llm_cb_;
    int interval_ = 3;
    bool active_ = false;

    std::string build_prompt(const WorldGraph& graph,
                             const std::string& scene_context) const;
    WeaveResult parse_and_apply(WorldGraph& graph,
                                const std::string& llm_response,
                                int turn_index);
    WeaveResult weave_impl(WorldGraph& graph, int turn_index,
                           const std::string& scene_context);

    // Selection RNG (mutable: not part of observable Weaver state)
    mutable std::mt19937 rng_{std::random_device{}()};

    // Expiry state
    std::atomic<bool> expiry_stop_{false};
    std::vector<std::vector<std::uint64_t>> expiry_queue_;
    std::vector<ExpiryOp> check_group(WorldGraph& graph,
                                      std::vector<const Node*> live_nodes,
                                      int turn_index);
};

}  // namespace rhapsode
