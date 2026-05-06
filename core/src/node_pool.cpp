#include "rhapsode/node_pool.h"
#include <algorithm>

namespace rhapsode {

Node& NodePool::add(Node node) {
    if (node.id == 0) {
        node.id = next_id_++;
    } else if (node.id >= next_id_) {
        next_id_ = node.id + 1;
    }

    auto id = node.id;
    nodes_[id] = std::move(node);
    return nodes_.at(id);
}

Node* NodePool::get(std::uint64_t id) {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

const Node* NodePool::get(std::uint64_t id) const {
    auto it = nodes_.find(id);
    return it != nodes_.end() ? &it->second : nullptr;
}

void NodePool::remove(std::uint64_t id) {
    nodes_.erase(id);
}

std::vector<Node*> NodePool::by_state(NodeState s) {
    std::vector<Node*> result;
    for (auto& [id, node] : nodes_) {
        if (node.state == s)
            result.push_back(&node);
    }
    return result;
}

std::vector<Node*> NodePool::by_entity(const std::string& entity_id) {
    std::vector<Node*> result;
    for (auto& [id, node] : nodes_) {
        for (const auto& entity : node.entities) {
            if (entity == entity_id) {
                result.push_back(&node);
                break;
            }
        }
    }
    return result;
}

std::vector<Node*> NodePool::by_known_by(const std::string& who) {
    std::vector<Node*> result;
    for (auto& [id, node] : nodes_) {
        for (const auto& knower : node.known_by) {
            if (knower == who) {
                result.push_back(&node);
                break;
            }
        }
    }
    return result;
}

std::vector<Node*> NodePool::wavefront() {
    return by_state(NodeState::Active);
}

void NodePool::for_each(VisitFn fn) const {
    for (const auto& [id, node] : nodes_)
        fn(node);
}

std::vector<Node> NodePool::all_nodes() const {
    std::vector<Node> result;
    result.reserve(nodes_.size());
    for (const auto& [id, node] : nodes_)
        result.push_back(node);
    return result;
}

// --- Serialization ---

nlohmann::json NodePool::to_json() const {
    std::vector<std::uint64_t> sorted_ids;
    sorted_ids.reserve(nodes_.size());
    for (const auto& [id, node] : nodes_)
        sorted_ids.push_back(id);
    std::sort(sorted_ids.begin(), sorted_ids.end());

    nlohmann::json arr = nlohmann::json::array();
    for (auto id : sorted_ids)
        arr.push_back(nodes_.at(id));

    return nlohmann::json{{"next_id", next_id_}, {"nodes", std::move(arr)}};
}

NodePool NodePool::from_json(const nlohmann::json& j) {
    NodePool pool;
    pool.next_id_ = j.value("next_id", std::uint64_t{1});

    auto it = j.find("nodes");
    if (it != j.end() && it->is_array()) {
        for (const auto& item : *it)
            pool.add(item.get<Node>());
    }

    if (pool.next_id_ == 0)
        pool.next_id_ = 1;

    return pool;
}

} // namespace rhapsode
