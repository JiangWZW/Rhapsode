#include "rhapsode/world_graph.h"

#include <utility>

namespace rhapsode {

nlohmann::json WorldGraph::to_json() const {
    nlohmann::json j;
    j["next_id"] = next_id_;

    nlohmann::json nodes = nlohmann::json::array();
    for_each([&](const Node& node) { nodes.push_back(node); }, true);
    j["nodes"] = std::move(nodes);

    nlohmann::json edges = nlohmann::json::array();
    auto [edge_it, edge_end] = boost::edges(graph_);
    for (; edge_it != edge_end; ++edge_it) {
        Vertex src = boost::source(*edge_it, graph_);
        Vertex dst = boost::target(*edge_it, graph_);
        const EdgeData& data = graph_[*edge_it];
        nlohmann::json e = {
            {"from", graph_[src].id},
            {"to", graph_[dst].id},
            {"weight", data.weight},
            {"created_at", data.created_at},
            {"active", data.active},
        };
        if (!data.kind.empty()) e["kind"] = data.kind;
        edges.push_back(std::move(e));
    }
    j["edges"] = std::move(edges);

    return j;
}

WorldGraph WorldGraph::from_json(const nlohmann::json& j) {
    WorldGraph g;
    g.next_id_ = j.value("next_id", std::uint64_t{1});

    for (const auto& node_j : j.value("nodes", nlohmann::json::array())) {
        Node node = node_j.get<Node>();
        g.add_node(std::move(node));
    }

    for (const auto& edge_j : j.value("edges", nlohmann::json::array())) {
        std::uint64_t from = edge_j.value("from", std::uint64_t{0});
        std::uint64_t to = edge_j.value("to", std::uint64_t{0});
        float weight = edge_j.value("weight", edge_j.value("confidence", 1.0f));
        int created_at = edge_j.value("created_at", 0);
        bool active = edge_j.value("active", true);
        std::string kind = edge_j.value("kind", std::string());

        if (g.add_relation(from, to, weight, created_at, kind)) {
            auto from_it = g.id_to_vertex_.find(from);
            auto to_it = g.id_to_vertex_.find(to);
            if (from_it != g.id_to_vertex_.end() && to_it != g.id_to_vertex_.end()) {
                auto [edge, ok] = boost::edge(from_it->second, to_it->second, g.graph_);
                if (ok) g.graph_[edge].active = active;
            }
        }
    }

    return g;
}

WorldGraph WorldGraph::from_legacy_node_pool_json(const nlohmann::json& j) {
    WorldGraph g;
    g.next_id_ = j.value("next_id", std::uint64_t{1});
    for (const auto& node_j : j.value("nodes", nlohmann::json::array())) {
        Node node = node_j.get<Node>();
        g.add_node(std::move(node));
    }
    return g;
}

}  // namespace rhapsode
