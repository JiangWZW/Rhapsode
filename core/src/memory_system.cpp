#include "rhapsode/memory_system.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

namespace rhapsode {

MemorySystem::MemorySystem(const std::string& scene_id)
    : scene_id_(scene_id)
    , collection_(scene_id + "_nodes")
{}

// -- Callback setters --

void MemorySystem::set_embed_callback(EmbedCallback cb)            { embed_cb_ = std::move(cb); }
void MemorySystem::set_store_callback(StoreCallback cb)            { store_cb_ = std::move(cb); }
void MemorySystem::set_query_callback(QueryCallback cb)            { query_cb_ = std::move(cb); }
void MemorySystem::set_update_meta_callback(UpdateMetaCallback cb) { update_meta_cb_ = std::move(cb); }
void MemorySystem::set_get_by_meta_callback(GetByMetaCallback cb)  { get_by_meta_cb_ = std::move(cb); }
void MemorySystem::set_delete_callback(DeleteCallback cb)          { delete_cb_ = std::move(cb); }
void MemorySystem::set_local_llm_callback(LocalLLMCallback cb)     { local_llm_cb_ = std::move(cb); }

// -- Delete --

void MemorySystem::delete_nodes(const std::vector<std::uint64_t>& node_ids) {
    if (!delete_cb_ || node_ids.empty()) return;
    nlohmann::json ids = nlohmann::json::array();
    for (auto id : node_ids) ids.push_back("node_" + std::to_string(id));
    delete_cb_(collection_, ids.dump());
    std::cerr << "  [memory] Deleted " << node_ids.size() << " nodes from ChromaDB\n" << std::flush;
}

// -- Write --

void MemorySystem::store_node(std::uint64_t node_id,
                              const std::string& fact,
                              const std::string& state,
                              const std::string& type,
                              int turn) {
    if (!embed_cb_) throw std::runtime_error("No embed callback registered");
    if (!store_cb_) throw std::runtime_error("No store callback registered");

    auto embedding_json = embed_cb_(fact);
    std::string doc_id = "node_" + std::to_string(node_id);

    nlohmann::json metadata = {
        {"node_id",    node_id},
        {"created_at", turn},
        {"state",      state},
        {"type",       type},
    };

    store_cb_(collection_, doc_id, fact, embedding_json, metadata.dump());
}

// -- Read --

std::vector<std::uint64_t> MemorySystem::search_nodes(const std::string& query,
                                                       int top_k) const {
    if (!embed_cb_ || !query_cb_) return {};

    auto query_emb_json = embed_cb_(query);
    nlohmann::json where = {{"state", {{"$ne", "dormant"}}}};
    auto raw_str = query_cb_(collection_, query_emb_json, top_k, where.dump());
    auto raw = nlohmann::json::parse(raw_str);

    std::vector<std::uint64_t> result;
    auto ids = raw.value("ids", nlohmann::json::array());
    if (ids.empty() || ids[0].empty()) return result;

    auto& metadatas = raw["metadatas"][0];
    result.reserve(metadatas.size());
    for (const auto& meta : metadatas) {
        if (meta.contains("node_id"))
            result.push_back(meta["node_id"].get<std::uint64_t>());
    }
    return result;
}

// -- Pipeline --

void MemorySystem::process_new_nodes(const std::vector<Node>& nodes, int turn) {
    if (nodes.empty()) return;

    int stored = 0;
    for (const auto& n : nodes) {
        if (n.id == 0) continue;
        store_node(n.id, n.fact, to_string(n.state), n.type, turn);
        ++stored;
    }

    if (stored > 0)
        std::cerr << "  [memory] Indexed " << stored << " nodes in ChromaDB\n" << std::flush;
}

void MemorySystem::sync_expired(const std::vector<Node>& expired_nodes) {
    if (!update_meta_cb_) return;

    int synced = 0;
    for (const auto& node : expired_nodes) {
        std::string doc_id = "node_" + std::to_string(node.id);
        nlohmann::json meta = {
            {"valid_until", node.valid_until},
        };
        update_meta_cb_(collection_, doc_id, meta.dump());
        ++synced;
    }

    if (synced > 0)
        std::cerr << "  [memory] Synced " << synced << " expired nodes in ChromaDB\n" << std::flush;
}

} // namespace rhapsode
