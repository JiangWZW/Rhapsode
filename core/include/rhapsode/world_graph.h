#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

#include "rhapsode/node.h"

namespace rhapsode {

enum class RelationKind {
    Related,
    Supersedes,
    Contradicts,
    CausedBy,
};

struct EdgeData {
    RelationKind kind = RelationKind::Related;
    float confidence = 1.0f;
    int created_at = 0;
    bool active = true;
};

std::string to_string(RelationKind k);
RelationKind relation_kind_from_string(const std::string& s);

class WorldGraph {
public:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Node, EdgeData>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;

    Node& add_node(Node node);
    void upsert_node(const Node& node);
    bool has_node(std::uint64_t node_id) const;
    Node* get_node(std::uint64_t node_id);
    const Node* get_node(std::uint64_t node_id) const;
    bool mark_resolved(std::uint64_t node_id, int resolved_at);
    std::size_t size() const;
    std::vector<Node> all_nodes(bool include_resolved = false) const;
    void for_each(std::function<void(const Node&)> fn, bool include_resolved = false) const;

    bool add_relation(std::uint64_t from_id,
                      std::uint64_t to_id,
                      RelationKind kind = RelationKind::Related,
                      float confidence = 1.0f,
                      int created_at = 0);
    std::vector<std::uint64_t> neighbors(
        std::uint64_t node_id,
        std::optional<RelationKind> kind_filter = std::nullopt) const;
    std::vector<std::uint64_t> neighbors_within(
        std::uint64_t source_id,
        int max_hops,
        std::optional<RelationKind> relation_filter = std::nullopt,
        bool active_only = true) const;

    nlohmann::json to_json() const;
    static WorldGraph from_json(const nlohmann::json& j);
    static WorldGraph from_legacy_node_pool_json(const nlohmann::json& j);

private:
    Graph graph_;
    std::unordered_map<std::uint64_t, Vertex> id_to_vertex_;
    std::uint64_t next_id_ = 1;
};

}  // namespace rhapsode
