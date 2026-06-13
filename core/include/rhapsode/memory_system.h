#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "rhapsode/node.h"

namespace rhapsode {

using EmbedCallback      = std::function<std::string(const std::string& text)>;
using StoreCallback      = std::function<void(const std::string& collection,
                                              const std::string& id,
                                              const std::string& doc,
                                              const std::string& embedding_json,
                                              const std::string& metadata_json)>;
using QueryCallback      = std::function<std::string(const std::string& collection,
                                                     const std::string& embedding_json,
                                                     int n,
                                                     const std::string& where_json)>;
using UpdateMetaCallback = std::function<void(const std::string& collection,
                                              const std::string& id,
                                              const std::string& metadata_json)>;
using GetByMetaCallback  = std::function<std::string(const std::string& collection,
                                                     const std::string& where_json)>;
using DeleteCallback     = std::function<void(const std::string& collection,
                                              const std::string& ids_json)>;
using LocalLLMCallback   = std::function<std::string(const std::string& prompt)>;

class MemorySystem {
public:
    explicit MemorySystem(const std::string& scene_id);

    void set_embed_callback(EmbedCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_update_meta_callback(UpdateMetaCallback cb);
    void set_get_by_meta_callback(GetByMetaCallback cb);
    void set_delete_callback(DeleteCallback cb);
    void set_local_llm_callback(LocalLLMCallback cb);

    void delete_nodes(const std::vector<std::uint64_t>& node_ids);

    void store_node(std::uint64_t node_id,
                    const std::string& fact,
                    const std::string& state,
                    const std::string& type,
                    int turn);

    std::vector<std::uint64_t> search_nodes(const std::string& query,
                                            int top_k = 10) const;

    void process_new_nodes(const std::vector<Node>& nodes, int turn);
    void sync_expired(const std::vector<Node>& expired_nodes);

    int get_next_id() const { return next_id_; }
    void set_next_id(int id) { next_id_ = id; }

private:
    std::string scene_id_;
    std::string collection_;
    int next_id_ = 0;

    EmbedCallback      embed_cb_;
    StoreCallback      store_cb_;
    QueryCallback      query_cb_;
    UpdateMetaCallback update_meta_cb_;
    GetByMetaCallback  get_by_meta_cb_;
    DeleteCallback     delete_cb_;
    LocalLLMCallback   local_llm_cb_;
};

} // namespace rhapsode
