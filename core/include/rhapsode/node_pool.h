#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "rhapsode/node.h"

namespace rhapsode {

class NodePool {
public:
    Node& add(Node node);
    Node* get(std::uint64_t id);
    const Node* get(std::uint64_t id) const;
    void remove(std::uint64_t id);
    std::size_t size() const { return nodes_.size(); }

    std::vector<Node*> by_state(NodeState s);
    std::vector<Node*> by_entity(const std::string& entity_id);
    std::vector<Node*> by_known_by(const std::string& who);
    std::vector<Node*> wavefront();

    using VisitFn = std::function<void(const Node&)>;
    void for_each(VisitFn fn) const;

    // Convenience: copies all nodes into a vector. Prefer for_each() in hot paths.
    std::vector<Node> all_nodes() const;

    nlohmann::json to_json() const;
    static NodePool from_json(const nlohmann::json& j);

private:
    std::unordered_map<std::uint64_t, Node> nodes_;
    std::uint64_t next_id_ = 1;
};

} // namespace rhapsode
