#include "rhapsode/world_graph.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <set>
#include <stdexcept>

namespace rhapsode {

std::string to_string(RelationKind k) {
    switch (k) {
        case RelationKind::Related: return "related";
        case RelationKind::Supersedes: return "supersedes";
        case RelationKind::Contradicts: return "contradicts";
        case RelationKind::CausedBy: return "caused_by";
    }
    throw std::runtime_error("Invalid RelationKind");
}

RelationKind relation_kind_from_string(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "related") return RelationKind::Related;
    if (lower == "supersedes") return RelationKind::Supersedes;
    if (lower == "contradicts") return RelationKind::Contradicts;
    if (lower == "caused_by") return RelationKind::CausedBy;
    throw std::runtime_error("Invalid relation kind string: " + s);
}

Node& WorldGraph::add_node(Node node) {
    if (node.id == 0) {
        node.id = next_id_++;
    } else if (node.id >= next_id_) {
        next_id_ = node.id + 1;
    }

    auto it = id_to_vertex_.find(node.id);
    if (it != id_to_vertex_.end()) {
        graph_[it->second] = std::move(node);
        return graph_[it->second];
    }

    Vertex v = boost::add_vertex(graph_);
    graph_[v] = std::move(node);
    id_to_vertex_[graph_[v].id] = v;
    return graph_[v];
}

void WorldGraph::upsert_node(const Node& node) {
    add_node(node);
}

bool WorldGraph::has_node(std::uint64_t node_id) const {
    return id_to_vertex_.find(node_id) != id_to_vertex_.end();
}

Node* WorldGraph::get_node(std::uint64_t node_id) {
    auto it = id_to_vertex_.find(node_id);
    if (it == id_to_vertex_.end()) return nullptr;
    return &graph_[it->second];
}

const Node* WorldGraph::get_node(std::uint64_t node_id) const {
    auto it = id_to_vertex_.find(node_id);
    if (it == id_to_vertex_.end()) return nullptr;
    return &graph_[it->second];
}

bool WorldGraph::mark_resolved(std::uint64_t node_id, int resolved_at) {
    Node* node = get_node(node_id);
    if (!node) return false;
    node->state = NodeState::Resolved;
    node->resolved_at = resolved_at;
    return true;
}

std::size_t WorldGraph::size() const {
    std::size_t count = 0;
    for_each([&](const Node&) { ++count; }, false);
    return count;
}

std::vector<Node> WorldGraph::all_nodes(bool include_resolved) const {
    std::vector<Node> nodes;
    for_each([&](const Node& n) { nodes.push_back(n); }, include_resolved);
    return nodes;
}

void WorldGraph::for_each(std::function<void(const Node&)> fn, bool include_resolved) const {
    for (const auto& [id, v] : id_to_vertex_) {
        (void)id;
        const Node& node = graph_[v];
        if (!include_resolved && node.state == NodeState::Resolved) {
            continue;
        }
        fn(node);
    }
}

bool WorldGraph::add_relation(std::uint64_t from_id,
                              std::uint64_t to_id,
                              RelationKind kind,
                              float confidence,
                              int created_at) {
    auto from_it = id_to_vertex_.find(from_id);
    auto to_it = id_to_vertex_.find(to_id);
    if (from_it == id_to_vertex_.end() || to_it == id_to_vertex_.end()) {
        return false;
    }

    auto [existing_it, existing_end] = boost::out_edges(from_it->second, graph_);
    for (; existing_it != existing_end; ++existing_it) {
        Vertex target = boost::target(*existing_it, graph_);
        if (graph_[target].id == to_id && graph_[*existing_it].kind == kind) {
            return false;
        }
    }

    auto [edge, inserted] = boost::add_edge(from_it->second, to_it->second, graph_);
    graph_[edge].kind = kind;
    graph_[edge].confidence = confidence;
    graph_[edge].created_at = created_at;
    graph_[edge].active = true;
    return inserted;
}

std::vector<std::uint64_t> WorldGraph::neighbors(std::uint64_t node_id,
                                                 std::optional<RelationKind> kind_filter) const {
    std::vector<std::uint64_t> result;
    auto it = id_to_vertex_.find(node_id);
    if (it == id_to_vertex_.end()) {
        return result;
    }

    auto [edge_it, edge_end] = boost::out_edges(it->second, graph_);
    for (; edge_it != edge_end; ++edge_it) {
        const EdgeData& edge_data = graph_[*edge_it];
        if (!edge_data.active) {
            continue;
        }
        if (kind_filter.has_value() && edge_data.kind != *kind_filter) {
            continue;
        }

        Vertex target = boost::target(*edge_it, graph_);
        result.push_back(graph_[target].id);
    }

    return result;
}

std::vector<std::uint64_t> WorldGraph::neighbors_within(
    std::uint64_t source_id,
    int max_hops,
    std::optional<RelationKind> relation_filter,
    bool active_only) const {
    std::vector<std::uint64_t> result;
    if (max_hops < 0) return result;

    auto src_it = id_to_vertex_.find(source_id);
    if (src_it == id_to_vertex_.end()) return result;

    std::deque<std::pair<Vertex, int>> queue;
    std::set<Vertex> visited;
    queue.push_back({src_it->second, 0});
    visited.insert(src_it->second);

    while (!queue.empty()) {
        auto [current, depth] = queue.front();
        queue.pop_front();
        if (depth == max_hops) continue;

        auto [edge_it, edge_end] = boost::out_edges(current, graph_);
        for (; edge_it != edge_end; ++edge_it) {
            const EdgeData& edge_data = graph_[*edge_it];
            if (active_only && !edge_data.active) continue;
            if (relation_filter.has_value() && edge_data.kind != *relation_filter) continue;

            Vertex target = boost::target(*edge_it, graph_);
            if (visited.insert(target).second) {
                result.push_back(graph_[target].id);
                queue.push_back({target, depth + 1});
            }
        }
    }

    return result;
}

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
        edges.push_back({
            {"from", graph_[src].id},
            {"to", graph_[dst].id},
            {"kind", to_string(data.kind)},
            {"confidence", data.confidence},
            {"created_at", data.created_at},
            {"active", data.active},
        });
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
        RelationKind kind = relation_kind_from_string(edge_j.value("kind", "related"));
        float confidence = edge_j.value("confidence", 1.0f);
        int created_at = edge_j.value("created_at", 0);
        bool active = edge_j.value("active", true);

        if (g.add_relation(from, to, kind, confidence, created_at)) {
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
