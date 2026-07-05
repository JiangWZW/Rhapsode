#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

#include "rhapsode/node.h"

namespace rhapsode {

struct EdgeData {
    float weight = 1.0f;
    int created_at = 0;
    bool active = true;
    // Relation kind: "" / "chain" = entity-timeline predecessor link;
    // "evidence" = a Thought extended/supported by this node;
    // "tension" = a contradiction kept live (never collapsed).
    std::string kind;
};

struct EdgeInfo {
    std::uint64_t from_id;
    std::uint64_t to_id;
    EdgeData data;
};

class WorldGraph {
public:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Node, EdgeData>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;

    Node& add_node(Node node);
    /// Insert a node AND link it to the most-recent active node per shared
    /// entity (the temporal-chaining invariant: a fact links to what it
    /// follows). Use for live insertions; from_json uses raw add_node so it
    /// does not re-chain over its explicitly-restored edges.
    Node& add_node_chained(Node node, int turn);
    bool has_node(std::uint64_t node_id) const;
    Node* get_node(std::uint64_t node_id);
    const Node* get_node(std::uint64_t node_id) const;
    bool set_valid_until(std::uint64_t node_id, int valid_until_turn);
    std::size_t size() const;
    int active_degree(std::uint64_t node_id) const;
    std::vector<Node> all_nodes(bool include_expired = false) const;
    void for_each(const std::function<void(const Node&)>& fn,
                  bool include_expired = false) const;

    bool add_relation(std::uint64_t from_id,
                      std::uint64_t to_id,
                      float weight = 1.0f,
                      int created_at = 0,
                      const std::string& kind = "");
    bool set_edge_active(std::uint64_t from_id, std::uint64_t to_id, bool active);
    bool set_edge_weight(std::uint64_t from_id, std::uint64_t to_id, float weight);
    bool set_edge_kind(std::uint64_t from_id, std::uint64_t to_id, const std::string& kind);
    std::vector<EdgeInfo> all_edges() const;

    std::vector<std::uint64_t> neighbors(std::uint64_t node_id) const;
    std::vector<std::uint64_t> neighbors_within(
        std::uint64_t source_id,
        int max_hops,
        bool active_only = true) const;

    std::vector<std::uint64_t> thread_containing(std::uint64_t seed_id) const;
    std::vector<std::vector<std::uint64_t>> all_threads() const;

    /// Active nodes grouped by entity string.
    /// Value = node IDs sorted by created_at descending (newest first).
    std::unordered_map<std::string, std::vector<std::uint64_t>>
        entity_groups() const;

    /// For a given node, find the most recent active predecessor per shared entity.
    /// Returns [(predecessor_id, entity_lowercase), ...] — deduplicated by pred ID.
    std::vector<std::pair<std::uint64_t, std::string>>
        chain_predecessors(const Node& new_node) const;

    std::vector<std::uint64_t> revert_to_turn(int turn);

    nlohmann::json to_json() const;
    std::string to_dot() const;
    static WorldGraph from_json(const nlohmann::json& j);
    static WorldGraph from_legacy_node_pool_json(const nlohmann::json& j);

private:
    Graph graph_;
    std::unordered_map<std::uint64_t, Vertex> id_to_vertex_;
    std::uint64_t next_id_ = 1;
};

}  // namespace rhapsode
