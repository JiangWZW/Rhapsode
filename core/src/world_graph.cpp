#include "rhapsode/world_graph.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <cctype>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace rhapsode {

namespace {

std::string to_lower_ascii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return s;
}

}  // namespace

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

Node& WorldGraph::add_node_chained(Node node, int turn) {
    node.related_to.clear();
    Node& ref = add_node(std::move(node));
    for (const auto& [pred_id, entity] : chain_predecessors(ref)) {
        (void)entity;
        if (add_relation(ref.id, pred_id, 1.0f, turn))
            ref.related_to.push_back(pred_id);
    }
    return ref;
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

bool WorldGraph::set_valid_until(std::uint64_t node_id, int valid_until_turn) {
    Node* node = get_node(node_id);
    if (!node) return false;
    node->valid_until = valid_until_turn;
    return true;
}

std::size_t WorldGraph::size() const {
    std::size_t count = 0;
    for_each([&](const Node&) { ++count; }, false);
    return count;
}

int WorldGraph::active_degree(std::uint64_t node_id) const {
    auto it = id_to_vertex_.find(node_id);
    if (it == id_to_vertex_.end()) return 0;
    Vertex v = it->second;
    int deg = 0;
    for (auto [eit, eend] = boost::out_edges(v, graph_); eit != eend; ++eit) {
        if (!graph_[*eit].active) continue;
        if (graph_[boost::target(*eit, graph_)].valid_until != -1) continue;
        ++deg;
    }
    for (auto [eit, eend] = boost::in_edges(v, graph_); eit != eend; ++eit) {
        if (!graph_[*eit].active) continue;
        if (graph_[boost::source(*eit, graph_)].valid_until != -1) continue;
        ++deg;
    }
    return deg;
}

std::vector<Node> WorldGraph::all_nodes(bool include_expired) const {
    std::vector<Node> nodes;
    nodes.reserve(id_to_vertex_.size());
    for_each([&](const Node& n) { nodes.push_back(n); }, include_expired);
    return nodes;
}

void WorldGraph::for_each(const std::function<void(const Node&)>& fn,
                          bool include_expired) const {
    for (const auto& [id, v] : id_to_vertex_) {
        (void)id;
        const Node& node = graph_[v];
        if (!include_expired && node.valid_until != -1)
            continue;
        fn(node);
    }
}

bool WorldGraph::add_relation(std::uint64_t from_id,
                              std::uint64_t to_id,
                              float weight,
                              int created_at) {
    auto from_it = id_to_vertex_.find(from_id);
    auto to_it = id_to_vertex_.find(to_id);
    if (from_it == id_to_vertex_.end() || to_it == id_to_vertex_.end())
        return false;

    // Enforce temporal direction: older node is always source
    const Node& from_node = graph_[from_it->second];
    const Node& to_node = graph_[to_it->second];
    if (from_node.created_at > to_node.created_at ||
        (from_node.created_at == to_node.created_at && from_id > to_id)) {
        std::swap(from_it, to_it);
        std::swap(from_id, to_id);
    }

    // Reject duplicate edges between the same pair
    auto [existing_it, existing_end] = boost::out_edges(from_it->second, graph_);
    for (; existing_it != existing_end; ++existing_it) {
        if (boost::target(*existing_it, graph_) == to_it->second)
            return false;
    }

    auto [edge, inserted] = boost::add_edge(from_it->second, to_it->second, graph_);
    graph_[edge].weight = weight;
    graph_[edge].created_at = created_at;
    graph_[edge].active = true;
    return inserted;
}

bool WorldGraph::set_edge_active(std::uint64_t from_id, std::uint64_t to_id, bool active) {
    auto fi = id_to_vertex_.find(from_id);
    auto ti = id_to_vertex_.find(to_id);
    if (fi == id_to_vertex_.end() || ti == id_to_vertex_.end()) return false;

    // Try both orderings since add_relation enforces temporal direction
    for (auto [s, t] : {std::pair{fi->second, ti->second},
                         std::pair{ti->second, fi->second}}) {
        auto [e, ok] = boost::edge(s, t, graph_);
        if (ok) { graph_[e].active = active; return true; }
    }
    return false;
}

bool WorldGraph::set_edge_weight(std::uint64_t from_id, std::uint64_t to_id, float weight) {
    auto fi = id_to_vertex_.find(from_id);
    auto ti = id_to_vertex_.find(to_id);
    if (fi == id_to_vertex_.end() || ti == id_to_vertex_.end()) return false;

    for (auto [s, t] : {std::pair{fi->second, ti->second},
                         std::pair{ti->second, fi->second}}) {
        auto [e, ok] = boost::edge(s, t, graph_);
        if (ok) { graph_[e].weight = weight; return true; }
    }
    return false;
}

std::vector<EdgeInfo> WorldGraph::all_edges() const {
    std::vector<EdgeInfo> result;
    auto [eit, eend] = boost::edges(graph_);
    for (; eit != eend; ++eit) {
        result.push_back({
            graph_[boost::source(*eit, graph_)].id,
            graph_[boost::target(*eit, graph_)].id,
            graph_[*eit]
        });
    }
    return result;
}

std::vector<std::uint64_t> WorldGraph::neighbors(std::uint64_t node_id) const {
    std::vector<std::uint64_t> result;
    auto it = id_to_vertex_.find(node_id);
    if (it == id_to_vertex_.end())
        return result;

    result.reserve(boost::out_degree(it->second, graph_));
    auto [edge_it, edge_end] = boost::out_edges(it->second, graph_);
    for (; edge_it != edge_end; ++edge_it) {
        if (!graph_[*edge_it].active)
            continue;
        result.push_back(graph_[boost::target(*edge_it, graph_)].id);
    }
    return result;
}

std::vector<std::uint64_t> WorldGraph::neighbors_within(
    std::uint64_t source_id,
    int max_hops,
    bool active_only) const {
    std::vector<std::uint64_t> result;
    if (max_hops < 0) return result;

    auto src_it = id_to_vertex_.find(source_id);
    if (src_it == id_to_vertex_.end()) return result;

    std::deque<std::pair<Vertex, int>> queue;
    std::unordered_set<Vertex> visited;
    visited.reserve(id_to_vertex_.size());
    queue.push_back({src_it->second, 0});
    visited.insert(src_it->second);

    while (!queue.empty()) {
        auto [current, depth] = queue.front();
        queue.pop_front();
        if (depth == max_hops) continue;

        auto [edge_it, edge_end] = boost::out_edges(current, graph_);
        for (; edge_it != edge_end; ++edge_it) {
            if (active_only && !graph_[*edge_it].active) continue;

            Vertex target = boost::target(*edge_it, graph_);
            if (visited.insert(target).second) {
                result.push_back(graph_[target].id);
                queue.push_back({target, depth + 1});
            }
        }
    }

    return result;
}

std::vector<std::uint64_t> WorldGraph::thread_containing(std::uint64_t seed_id) const {
    auto seed_it = id_to_vertex_.find(seed_id);
    if (seed_it == id_to_vertex_.end()) return {};

    // Build undirected view of active edges for connected_components
    using UndirectedView = boost::adjacency_list<boost::vecS, boost::vecS,
                                                  boost::undirectedS>;
    auto n_verts = boost::num_vertices(graph_);
    UndirectedView ug(n_verts);

    auto [eit, eend] = boost::edges(graph_);
    for (; eit != eend; ++eit) {
        if (!graph_[*eit].active) continue;
        auto s = boost::source(*eit, graph_);
        auto t = boost::target(*eit, graph_);
        boost::add_edge(s, t, ug);
    }

    std::vector<int> comp(n_verts);
    boost::connected_components(ug, comp.data());

    int target_comp = comp[seed_it->second];
    std::vector<std::uint64_t> thread;
    for (const auto& [id, v] : id_to_vertex_) {
        if (comp[v] == target_comp)
            thread.push_back(id);
    }
    std::sort(thread.begin(), thread.end());
    return thread;
}

std::vector<std::vector<std::uint64_t>> WorldGraph::all_threads() const {
    auto n_verts = boost::num_vertices(graph_);
    if (n_verts == 0) return {};

    using UndirectedView = boost::adjacency_list<boost::vecS, boost::vecS,
                                                  boost::undirectedS>;
    UndirectedView ug(n_verts);

    auto [eit, eend] = boost::edges(graph_);
    for (; eit != eend; ++eit) {
        if (!graph_[*eit].active) continue;
        auto s = boost::source(*eit, graph_);
        auto t = boost::target(*eit, graph_);
        boost::add_edge(s, t, ug);
    }

    std::vector<int> comp(n_verts);
    int num_comp = boost::connected_components(ug, comp.data());

    std::vector<std::vector<std::uint64_t>> threads(num_comp);
    for (const auto& [id, v] : id_to_vertex_)
        threads[comp[v]].push_back(id);

    // Remove empty components (vertices without ids) and sort each thread
    std::vector<std::vector<std::uint64_t>> result;
    result.reserve(num_comp);
    for (auto& t : threads) {
        if (t.empty()) continue;
        std::sort(t.begin(), t.end());
        result.push_back(std::move(t));
    }
    return result;
}

std::unordered_map<std::string, std::vector<std::uint64_t>>
WorldGraph::entity_groups() const {
    std::unordered_map<std::string, std::vector<std::uint64_t>> groups;
    for (const auto& [id, v] : id_to_vertex_) {
        const Node& node = graph_[v];
        if (node.state != NodeState::Active || node.valid_until != -1) continue;
        for (const auto& entity : node.entities)
            groups[entity].push_back(node.id);
    }
    for (auto& [entity, ids] : groups) {
        std::sort(ids.begin(), ids.end(),
                  [this](std::uint64_t a, std::uint64_t b) {
                      return graph_[id_to_vertex_.at(a)].created_at
                           > graph_[id_to_vertex_.at(b)].created_at;
                  });
    }
    return groups;
}

std::vector<std::pair<std::uint64_t, std::string>>
WorldGraph::chain_predecessors(const Node& new_node) const {
    std::unordered_set<std::string> target_entities;
    for (const auto& e : new_node.entities)
        target_entities.insert(to_lower_ascii(e));

    struct Best { std::uint64_t id = 0; int created_at = -1; };
    std::unordered_map<std::string, Best> most_recent;

    for (const auto& [id, v] : id_to_vertex_) {
        const Node& node = graph_[v];
        if (node.id == new_node.id) continue;
        if (node.state != NodeState::Active || node.valid_until != -1) continue;

        for (const auto& e : node.entities) {
            std::string key = to_lower_ascii(e);
            if (!target_entities.count(key)) continue;
            auto& best = most_recent[key];
            if (node.created_at > best.created_at ||
                (node.created_at == best.created_at && node.id > best.id)) {
                best = {node.id, node.created_at};
            }
        }
    }

    std::unordered_set<std::uint64_t> seen;
    std::vector<std::pair<std::uint64_t, std::string>> result;
    for (const auto& [entity, best] : most_recent) {
        if (best.id == 0) continue;
        if (seen.insert(best.id).second)
            result.emplace_back(best.id, entity);
    }
    return result;
}

std::vector<std::uint64_t> WorldGraph::revert_to_turn(int turn) {
    auto j = to_json();
    auto& nodes = j["nodes"];
    auto& edges = j["edges"];

    // Pass 1: partition nodes into alive/removed, reset valid_until where needed
    std::vector<std::uint64_t> removed;
    std::unordered_set<std::uint64_t> alive;
    for (auto it = nodes.begin(); it != nodes.end(); ) {
        if ((*it)["created_at"].get<int>() >= turn) {
            removed.push_back((*it)["id"].get<std::uint64_t>());
            it = nodes.erase(it);
        } else {
            alive.insert((*it)["id"].get<std::uint64_t>());
            if ((*it).value("valid_until", -1) >= turn)
                (*it)["valid_until"] = -1;
            ++it;
        }
    }

    // Pass 2: clean related_to (separate pass -- alive set incomplete during pass 1)
    for (auto& n : nodes) {
        if (!n.contains("related_to")) continue;
        auto& rel = n["related_to"];
        for (auto ri = rel.begin(); ri != rel.end(); )
            ri = alive.count(ri->get<std::uint64_t>()) ? std::next(ri) : rel.erase(ri);
    }

    // Pass 3: filter edges
    for (auto it = edges.begin(); it != edges.end(); ) {
        if (!alive.count((*it)["from"].get<std::uint64_t>()) ||
            !alive.count((*it)["to"].get<std::uint64_t>()) ||
            (*it)["created_at"].get<int>() >= turn) {
            it = edges.erase(it);
        } else {
            ++it;
        }
    }

    *this = WorldGraph::from_json(j);
    return removed;
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
            {"weight", data.weight},
            {"created_at", data.created_at},
            {"active", data.active},
        });
    }
    j["edges"] = std::move(edges);

    return j;
}

// Render a fact as a graphviz label: full text (no truncation), soft-wrapped at
// word boundaries so node boxes stay readable, and always valid UTF-8.
static std::string dot_escape(const std::string& s, std::size_t wrap_col = 45) {
    const std::string clean = sanitize_utf8(s);  // never split a multibyte char
    std::string out;
    out.reserve(clean.size() + 16);
    std::size_t col = 0;
    for (char c : clean) {
        switch (c) {
            case '"':  out += "\\\""; ++col; break;
            case '\\': out += "\\\\"; ++col; break;
            case '\r': break;
            case '\n': out += "\\n"; col = 0; break;
            case ' ':
                // wrap at the first space past the column limit
                if (col >= wrap_col) { out += "\\n"; col = 0; }
                else                 { out += ' ';   ++col; }
                break;
            default:   out += c; ++col; break;
        }
    }
    return out;
}

std::string WorldGraph::to_dot() const {
    std::ostringstream os;
    os << "digraph WorldGraph {\n"
       << "  rankdir=LR;\n"
       << "  bgcolor=\"#1e1e2e\";\n"
       << "  node [shape=box, style=\"filled,rounded\", fontname=\"Segoe UI\", fontsize=10];\n"
       << "  edge [fontname=\"Segoe UI\", fontsize=8, fontcolor=\"#cdd6f4\"];\n\n";

    for (const auto& [id, v] : id_to_vertex_) {
        const Node& n = graph_[v];
        const char* fill  = "#585b70";
        const char* font  = "#cdd6f4";
        const char* border = "#6c7086";
        switch (n.state) {
            case NodeState::Dormant:
                break;
            case NodeState::Foreshadowed:
                fill = "#f9e2af"; font = "#1e1e2e"; border = "#f2c678"; break;
            case NodeState::Active:
                if (n.valid_until != -1) {
                    fill = "#89b4fa"; font = "#1e1e2e"; border = "#6a9bf5";
                } else {
                    fill = "#a6e3a1"; font = "#1e1e2e"; border = "#74c76e";
                }
                break;
        }
        os << "  n" << n.id
           << " [label=\"[" << n.id << "] " << dot_escape(n.fact) << "\""
           << ", fillcolor=\"" << fill << "\""
           << ", fontcolor=\"" << font << "\""
           << ", color=\"" << border << "\""
           << "];\n";
    }

    os << "\n";

    auto [edge_it, edge_end] = boost::edges(graph_);
    for (; edge_it != edge_end; ++edge_it) {
        Vertex src = boost::source(*edge_it, graph_);
        Vertex dst = boost::target(*edge_it, graph_);
        const EdgeData& d = graph_[*edge_it];

        const char* style = d.active ? "solid" : "dashed";

        os << "  n" << graph_[src].id << " -> n" << graph_[dst].id
           << " [color=\"#a6adc8\""
           << ", style=" << style
           << "];\n";
    }

    os << "\n  label=\"Rhapsode World Graph\";\n"
       << "  fontname=\"Segoe UI\"; fontsize=14; fontcolor=\"#cdd6f4\";\n"
       << "}\n";
    return os.str();
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

        if (g.add_relation(from, to, weight, created_at)) {
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
