#pragma once
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include "rhapsode/node.h"

namespace rhapsode {

using EmbedCallback      = std::function<std::string(const std::string& text)>;
using LemmatizeCallback  = std::function<std::string(const std::string& text)>;
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
using LocalLLMCallback   = std::function<std::string(const std::string& prompt)>;

class MemorySystem {
public:
    explicit MemorySystem(const std::string& scene_id);

    void set_embed_callback(EmbedCallback cb);
    void set_lemmatize_callback(LemmatizeCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_update_meta_callback(UpdateMetaCallback cb);
    void set_get_by_meta_callback(GetByMetaCallback cb);
    void set_local_llm_callback(LocalLLMCallback cb);

    std::string store_fact(const std::string& fact,
                           const std::string& state,
                           const std::string& type,
                           const std::vector<std::string>& known_by,
                           const std::vector<std::string>& entities,
                           const std::vector<std::uint64_t>& related_to,
                           int turn);
    std::string store_fact(const std::string& fact,
                           const std::string& embedding_json,
                           const std::string& state,
                           const std::string& type,
                           const std::vector<std::string>& known_by,
                           const std::vector<std::string>& entities,
                           const std::vector<std::uint64_t>& related_to,
                           int turn);

    std::string retrieve(const std::string& query, int top_k = 8) const;
    std::string retrieve_for_injection(const std::string& scene_context,
                                       int max_results = 8) const;

    void process_new_nodes(const std::vector<Node>& nodes, int turn);
    void sync_resolved(const std::vector<Node>& resolved_nodes, int turn);

    int get_next_id() const { return next_id_; }
    void set_next_id(int id) { next_id_ = id; }

private:
    std::string scene_id_;
    std::string facts_collection_;
    std::string entities_collection_;
    int next_id_ = 0;

    EmbedCallback      embed_cb_;
    LemmatizeCallback  lemmatize_cb_;
    StoreCallback      store_cb_;
    QueryCallback      query_cb_;
    UpdateMetaCallback update_meta_cb_;
    GetByMetaCallback  get_by_meta_cb_;
    LocalLLMCallback   local_llm_cb_;

    static std::string md5_hash(const std::string& text);
    static std::vector<std::string> split_tokens(const std::string& text);
    static std::string join_strings(const std::vector<std::string>& v, char delim = ',');
    std::string next_node_id();

    bool is_duplicate(const std::string& hash) const;
    void link_entity(const std::string& entity_text, const std::string& memory_id);

    static float bm25_okapi(const std::vector<std::string>& query_terms,
                            const std::vector<std::string>& doc_terms,
                            const std::unordered_map<std::string, int>& corpus_df,
                            int corpus_size, float avgdl);
    static float normalize_bm25(float raw, float midpoint, float steepness);
    static std::pair<float, float> get_bm25_params(int num_terms);

    std::vector<std::string> distill_verbose(const std::string& fact);
    std::string score_quality_batch(const std::string& nodes_json,
                                    const std::string& pool_json);
    std::string extract_entities_batch(const std::string& nodes_json);
    bool detect_conflict(const std::string& fact,
                         const std::string& embedding_json) const;
};

} // namespace rhapsode
