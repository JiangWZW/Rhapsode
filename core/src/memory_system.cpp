#include "rhapsode/memory_system.h"
#include "rhapsode/md5.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <set>
#include <stdexcept>

namespace rhapsode {

static constexpr float ENTITY_LINK_THRESHOLD   = 0.95f;
static constexpr float ENTITY_SEARCH_THRESHOLD = 0.5f;
static constexpr float ENTITY_BOOST_WEIGHT     = 0.5f;
static constexpr float SEMANTIC_GATE           = 0.1f;
static constexpr float CONFLICT_SIM_THRESHOLD  = 0.85f;

MemorySystem::MemorySystem(const std::string& scene_id)
    : scene_id_(scene_id)
    , facts_collection_(scene_id + "_facts")
    , entities_collection_(scene_id + "_entities")
{}

// -- Callback setters --

void MemorySystem::set_embed_callback(EmbedCallback cb)            { embed_cb_ = std::move(cb); }
void MemorySystem::set_lemmatize_callback(LemmatizeCallback cb)    { lemmatize_cb_ = std::move(cb); }
void MemorySystem::set_store_callback(StoreCallback cb)            { store_cb_ = std::move(cb); }
void MemorySystem::set_query_callback(QueryCallback cb)            { query_cb_ = std::move(cb); }
void MemorySystem::set_update_meta_callback(UpdateMetaCallback cb) { update_meta_cb_ = std::move(cb); }
void MemorySystem::set_get_by_meta_callback(GetByMetaCallback cb)  { get_by_meta_cb_ = std::move(cb); }
void MemorySystem::set_local_llm_callback(LocalLLMCallback cb)     { local_llm_cb_ = std::move(cb); }

// -- Static utilities --

std::string MemorySystem::md5_hash(const std::string& text) {
    MD5 md5;
    md5.update(text.data(), text.size());
    md5.finalize();
    return md5.hexdigest();
}

std::vector<std::string> MemorySystem::split_tokens(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

std::string MemorySystem::join_strings(const std::vector<std::string>& v, char delim) {
    std::string result;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) result += delim;
        result += v[i];
    }
    return result;
}

std::string MemorySystem::next_node_id() {
    return "node_" + std::to_string(++next_id_);
}

bool MemorySystem::is_duplicate(const std::string& hash) const {
    if (!get_by_meta_cb_) return false;
    nlohmann::json where = {{"hash", hash}};
    auto result_json = get_by_meta_cb_(facts_collection_, where.dump());
    auto result = nlohmann::json::parse(result_json);
    return !result.value("ids", nlohmann::json::array()).empty();
}

// -- Write Path --

std::string MemorySystem::store_fact(
    const std::string& fact, const std::string& state,
    const std::string& type, const std::vector<std::string>& known_by,
    const std::vector<std::string>& entities,
    const std::vector<std::uint64_t>& related_to,
    int turn)
{
    if (!embed_cb_) throw std::runtime_error("No embed callback registered");
    auto embedding_json = embed_cb_(fact);
    return store_fact(fact, embedding_json, state, type, known_by, entities, related_to, turn);
}

std::string MemorySystem::store_fact(
    const std::string& fact, const std::string& embedding_json,
    const std::string& state, const std::string& type,
    const std::vector<std::string>& known_by,
    const std::vector<std::string>& entities,
    const std::vector<std::uint64_t>& related_to,
    int turn)
{
    if (!store_cb_) throw std::runtime_error("No store callback registered");

    auto hash = md5_hash(fact);
    if (is_duplicate(hash)) return "";

    std::string text_lemmatized = lemmatize_cb_ ? lemmatize_cb_(fact) : fact;

    auto node_id = next_node_id();
    std::vector<std::string> related_ids;
    related_ids.reserve(related_to.size());
    for (auto id : related_to) related_ids.push_back(std::to_string(id));
    nlohmann::json metadata = {
        {"hash",            hash},
        {"text_lemmatized", text_lemmatized},
        {"created_at",      turn},
        {"state",           state},
        {"type",            type},
        {"known_by",        join_strings(known_by)},
        {"entities",        join_strings(entities)},
        {"related_to",      join_strings(related_ids)},
    };

    store_cb_(facts_collection_, node_id, fact, embedding_json, metadata.dump());

    for (const auto& entity_text : entities)
        link_entity(entity_text, node_id);

    return node_id;
}

void MemorySystem::link_entity(const std::string& entity_text, const std::string& memory_id) {
    if (!embed_cb_ || !query_cb_ || !store_cb_) return;

    auto entity_emb_json = embed_cb_(entity_text);
    auto results_str = query_cb_(entities_collection_, entity_emb_json, 1, "{}");
    auto results = nlohmann::json::parse(results_str);

    auto ids = results.value("ids", nlohmann::json::array());
    if (!ids.empty() && !ids[0].empty()) {
        float dist = results["distances"][0][0].get<float>();
        if ((1.0f - dist) >= ENTITY_LINK_THRESHOLD) {
            std::string eid = ids[0][0].get<std::string>();
            auto meta = results["metadatas"][0][0];
            auto linked = nlohmann::json::parse(meta.value("linked_memory_ids", "[]"));
            if (std::find(linked.begin(), linked.end(), memory_id) == linked.end()) {
                linked.push_back(memory_id);
                meta["linked_memory_ids"] = linked.dump();
                if (update_meta_cb_) update_meta_cb_(entities_collection_, eid, meta.dump());
            }
            return;
        }
    }

    std::string eid = "ent_" + md5_hash(entity_text).substr(0, 12);
    nlohmann::json meta = {
        {"data", entity_text},
        {"linked_memory_ids", nlohmann::json::array({memory_id}).dump()},
    };
    store_cb_(entities_collection_, eid, entity_text, entity_emb_json, meta.dump());
}

// -- Read Path --

std::pair<float, float> MemorySystem::get_bm25_params(int num_terms) {
    if (num_terms <= 3)  return {5.0f, 0.7f};
    if (num_terms <= 6)  return {7.0f, 0.6f};
    if (num_terms <= 9)  return {9.0f, 0.5f};
    if (num_terms <= 15) return {10.0f, 0.5f};
    return {12.0f, 0.5f};
}

float MemorySystem::normalize_bm25(float raw, float midpoint, float steepness) {
    return 1.0f / (1.0f + std::exp(-steepness * (raw - midpoint)));
}

float MemorySystem::bm25_okapi(
    const std::vector<std::string>& query_terms,
    const std::vector<std::string>& doc_terms,
    const std::unordered_map<std::string, int>& corpus_df,
    int corpus_size, float avgdl)
{
    constexpr float k1 = 1.5f, b = 0.75f;
    float dl = static_cast<float>(doc_terms.size());

    std::unordered_map<std::string, int> tf_map;
    for (const auto& w : doc_terms) tf_map[w]++;

    float score = 0.0f;
    for (const auto& qt : query_terms) {
        auto it = corpus_df.find(qt);
        if (it == corpus_df.end()) continue;
        int df = it->second;

        float idf = std::log((corpus_size - df + 0.5f) / (df + 0.5f) + 1.0f);
        float tf = static_cast<float>(tf_map.count(qt) ? tf_map[qt] : 0);
        score += idf * (tf * (k1 + 1.0f)) / (tf + k1 * (1.0f - b + b * dl / avgdl));
    }
    return score;
}

std::string MemorySystem::retrieve(const std::string& query, int top_k) const {
    if (!embed_cb_ || !query_cb_) return "[]";

    std::string query_lemmatized = lemmatize_cb_ ? lemmatize_cb_(query) : query;
    auto query_tokens = split_tokens(query_lemmatized);
    if (query_tokens.empty()) return "[]";

    auto [midpoint, steepness] = get_bm25_params(static_cast<int>(query_tokens.size()));

    auto query_emb_json = embed_cb_(query);
    int n_fetch = std::max(top_k * 4, 60);
    nlohmann::json where = {{"state", {{"$ne", "dormant"}}}};

    auto raw_str = query_cb_(facts_collection_, query_emb_json, n_fetch, where.dump());
    auto raw = nlohmann::json::parse(raw_str);

    auto ids = raw.value("ids", nlohmann::json::array());
    if (ids.empty() || ids[0].empty()) return "[]";

    struct Candidate {
        std::string id;
        float semantic_score;
        std::string document;
        nlohmann::json metadata;
        std::vector<std::string> lemma_tokens;
    };
    std::vector<Candidate> candidates;
    std::unordered_map<std::string, int> corpus_df;
    float total_dl = 0.0f;

    for (size_t i = 0; i < ids[0].size(); ++i) {
        Candidate c;
        c.id = ids[0][i].get<std::string>();
        c.semantic_score = 1.0f - raw["distances"][0][i].get<float>();
        c.document = raw["documents"][0][i].get<std::string>();
        c.metadata = raw["metadatas"][0][i];
        c.lemma_tokens = split_tokens(c.metadata.value("text_lemmatized", ""));

        std::set<std::string> seen_in_doc;
        for (const auto& t : c.lemma_tokens) {
            if (seen_in_doc.insert(t).second)
                corpus_df[t]++;
        }
        total_dl += static_cast<float>(c.lemma_tokens.size());
        candidates.push_back(std::move(c));
    }

    int corpus_size = static_cast<int>(candidates.size());
    float avgdl = corpus_size > 0 ? total_dl / corpus_size : 1.0f;

    std::unordered_map<std::string, float> bm25_scores;
    for (const auto& c : candidates) {
        float raw_bm25 = bm25_okapi(query_tokens, c.lemma_tokens, corpus_df, corpus_size, avgdl);
        bm25_scores[c.id] = normalize_bm25(raw_bm25, midpoint, steepness);
    }

    std::unordered_map<std::string, float> entity_boosts;
    auto eres_str = query_cb_(entities_collection_, query_emb_json, 5, "{}");
    auto eres = nlohmann::json::parse(eres_str);

    auto eids = eres.value("ids", nlohmann::json::array());
    if (!eids.empty() && !eids[0].empty()) {
        std::set<std::string> candidate_ids;
        for (const auto& c : candidates) candidate_ids.insert(c.id);

        for (size_t j = 0; j < eids[0].size(); ++j) {
            float sim = 1.0f - eres["distances"][0][j].get<float>();
            if (sim < ENTITY_SEARCH_THRESHOLD) continue;

            auto meta = eres["metadatas"][0][j];
            auto linked = nlohmann::json::parse(meta.value("linked_memory_ids", "[]"));
            int n_linked = static_cast<int>(linked.size());
            float count_weight = 1.0f / (1.0f + 0.001f * std::pow(std::max(n_linked - 1, 0), 2));
            float boost = sim * ENTITY_BOOST_WEIGHT * count_weight;

            for (const auto& mid : linked) {
                std::string mid_str = mid.get<std::string>();
                if (candidate_ids.count(mid_str))
                    entity_boosts[mid_str] = std::max(entity_boosts[mid_str], boost);
            }
        }
    }

    float max_possible = 1.0f + 1.0f + (entity_boosts.empty() ? 0.0f : ENTITY_BOOST_WEIGHT);

    struct ScoredResult { std::string id; float score; std::string document; nlohmann::json metadata; };
    std::vector<ScoredResult> scored;
    scored.reserve(candidates.size());

    for (const auto& c : candidates) {
        if (c.semantic_score < SEMANTIC_GATE) continue;
        float bm25 = bm25_scores.count(c.id) ? bm25_scores[c.id] : 0.0f;
        float eboost = entity_boosts.count(c.id) ? entity_boosts[c.id] : 0.0f;
        float combined = std::min((c.semantic_score + bm25 + eboost) / max_possible, 1.0f);
        scored.push_back({c.id, combined, c.document, c.metadata});
    }

    std::partial_sort(scored.begin(),
                      scored.begin() + std::min(top_k, static_cast<int>(scored.size())),
                      scored.end(),
                      [](const auto& a, const auto& b) { return a.score > b.score; });
    if (static_cast<int>(scored.size()) > top_k)
        scored.resize(top_k);

    nlohmann::json output = nlohmann::json::array();
    for (const auto& s : scored) {
        nlohmann::json entry;
        entry["id"] = s.id;
        entry["score"] = s.score;
        entry["payload"] = s.metadata;
        entry["payload"]["fact"] = s.document;
        output.push_back(entry);
    }
    return output.dump();
}

std::string MemorySystem::retrieve_for_injection(const std::string& scene_context, int max_results) const {
    if (scene_context.empty()) return "[]";
    auto results_str = retrieve(scene_context, max_results);
    auto results = nlohmann::json::parse(results_str);
    nlohmann::json facts = nlohmann::json::array();
    for (const auto& r : results)
        if (r.contains("payload") && r["payload"].contains("fact"))
            facts.push_back(r["payload"]["fact"]);
    return facts.dump();
}

// -- Pipeline sub-steps --

std::vector<std::string> MemorySystem::distill_verbose(const std::string& fact) {
    if (static_cast<int>(split_tokens(fact).size()) <= 25) return {fact};
    if (!local_llm_cb_) return {fact};

    std::string prompt =
        "Rewrite this verbose fact as 1-3 short atomic facts (max 15 words each).\n"
        "Keep all information. No articles, no hedging.\n\n"
        "Input: \"" + fact + "\"\n\n"
        "Output as JSON array of strings:";

    auto raw = local_llm_cb_(prompt);
    if (raw.empty()) return {fact};

    try {
        auto parsed = nlohmann::json::parse(raw);
        if (!parsed.is_array()) return {fact};
        std::vector<std::string> results;
        for (const auto& item : parsed)
            if (item.is_string() && !item.get<std::string>().empty())
                results.push_back(item.get<std::string>());
        return results.empty() ? std::vector<std::string>{fact} : results;
    } catch (...) {
        return {fact};
    }
}

std::string MemorySystem::score_quality_batch(const std::string& nodes_json,
                                              const std::string& pool_json) {
    if (!local_llm_cb_) return nodes_json;

    std::string prompt =
        "You are judging world-state facts for a text RPG memory system.\n"
        "Score each fact 1-5 on INFORMATION QUALITY for later retrieval.\n\n"
        "1 = vague or meaningless (\"something happened\")\n"
        "2 = names an entity but predicate is trivial or redundant with pool\n"
        "3 = meaningful but poorly worded -- ambiguous, missing context, or not self-contained\n"
        "4 = good -- specific, atomic, self-contained, names concrete entities\n"
        "5 = excellent -- precise, unambiguous, useful for future story continuity\n\n"
        "For score 3 ONLY, provide a rewrite that fixes the wording issue.\n"
        "REWRITE RULES:\n"
        "- Use ONLY information already in the original fact. Do NOT invent details.\n"
        "- Max 15 words. Keep it terse and atomic.\n"
        "- Goal: make the fact self-contained and unambiguous, nothing more.\n\n"
        "Existing pool (for redundancy check):\n" + pool_json + "\n\n"
        "Facts to judge:\n" + nodes_json + "\n\n"
        "Respond as JSON array:\n"
        "[{\"index\": 0, \"score\": N, \"reason\": \"...\", \"rewrite\": null}, ...]";

    auto raw = local_llm_cb_(prompt);
    if (raw.empty()) {
        std::cerr << "  [quality] LLM returned empty response, skipping scoring\n" << std::flush;
        return nodes_json;
    }
    std::cerr << "  [quality] LLM raw response (" << raw.size() << " chars): "
              << raw.substr(0, 300) << (raw.size() > 300 ? "..." : "") << "\n";

    try {
        auto scores = nlohmann::json::parse(raw);
        auto nodes = nlohmann::json::parse(nodes_json);
        nlohmann::json accepted = nlohmann::json::array();

        std::unordered_map<int, nlohmann::json> score_map;
        for (const auto& s : scores)
            score_map[s.value("index", -1)] = s;

        for (size_t i = 0; i < nodes.size(); ++i) {
            auto it = score_map.find(static_cast<int>(i));
            int quality = 4;
            std::string reason;
            std::string fact_text = nodes[i].value("fact", "");
            if (it != score_map.end()) {
                quality = it->second.value("score", 4);
                reason = it->second.value("reason", "");
            }

            if (quality >= 4) {
                std::cerr << "  [quality] PASS (" << quality << "): \"" << fact_text << "\"";
                if (!reason.empty()) std::cerr << " -- " << reason;
                std::cerr << "\n";
            } else if (quality == 3) {
                if (it != score_map.end() && it->second.contains("rewrite") &&
                    !it->second["rewrite"].is_null()) {
                    std::string rewrite = it->second["rewrite"].get<std::string>();
                    int orig_words = static_cast<int>(split_tokens(fact_text).size());
                    int rewrite_words = static_cast<int>(split_tokens(rewrite).size());
                    int max_words = std::min(orig_words + 1, 50);
                    if (rewrite_words <= max_words && !rewrite.empty()) {
                        nodes[i]["fact"] = rewrite;
                        std::cerr << "  [quality] REWRITE (3): \"" << fact_text
                                  << "\" -> \"" << rewrite << "\"";
                        if (!reason.empty()) std::cerr << " -- " << reason;
                        std::cerr << "\n";
                    } else {
                        std::cerr << "  [quality] REWRITE REJECTED (" << rewrite_words
                                  << "w > " << max_words << "w max): \""
                                  << rewrite << "\"\n";
                        std::cerr << "  [quality] KEEP (3): \"" << fact_text << "\"\n";
                    }
                } else {
                    std::cerr << "  [quality] PASS (3, no rewrite): \"" << fact_text << "\"";
                    if (!reason.empty()) std::cerr << " -- " << reason;
                    std::cerr << "\n";
                }
            }

            if (quality >= 3) {
                nodes[i]["quality_score"] = quality;
                accepted.push_back(nodes[i]);
            } else {
                std::cerr << "  [quality] REJECTED (" << quality << "): \""
                          << fact_text << "\"";
                if (!reason.empty()) std::cerr << " -- " << reason;
                std::cerr << "\n";
            }
        }
        std::cerr << std::flush;
        return accepted.dump();
    } catch (const std::exception& e) {
        std::cerr << "  [quality] PARSE FAILED: " << e.what() << "\n"
                  << "  [quality] Raw was: " << raw.substr(0, 500) << "\n" << std::flush;
        return nodes_json;
    }
}

std::string MemorySystem::extract_entities_batch(const std::string& nodes_json) {
    auto nodes = nlohmann::json::parse(nodes_json);
    if (!local_llm_cb_) {
        for (auto& n : nodes) n["entities"] = nlohmann::json::array();
        return nodes.dump();
    }

    nlohmann::json facts_arr = nlohmann::json::array();
    for (size_t i = 0; i < nodes.size(); ++i)
        facts_arr.push_back({{"index", i}, {"fact", nodes[i].value("fact", "")}});

    std::string prompt =
        "Extract named entities (characters, locations, items, organizations, "
        "creatures) from each RPG fact.\n\n"
        "Facts:\n" + facts_arr.dump() + "\n\n"
        "Respond as JSON array:\n"
        "[{\"index\": 0, \"entities\": [\"entity1\", \"entity2\"]}, ...]";

    auto raw = local_llm_cb_(prompt);
    if (raw.empty()) {
        for (auto& n : nodes) n["entities"] = nlohmann::json::array();
        return nodes.dump();
    }

    try {
        auto parsed = nlohmann::json::parse(raw);
        std::unordered_map<int, std::vector<std::string>> entity_map;
        for (const auto& e : parsed) {
            std::vector<std::string> ents;
            for (const auto& ent : e.value("entities", nlohmann::json::array()))
                ents.push_back(ent.get<std::string>());
            entity_map[e.value("index", -1)] = std::move(ents);
        }
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto it = entity_map.find(static_cast<int>(i));
            nodes[i]["entities"] = (it != entity_map.end())
                ? nlohmann::json(it->second) : nlohmann::json::array();
        }
    } catch (...) {
        for (auto& n : nodes) n["entities"] = nlohmann::json::array();
    }
    return nodes.dump();
}

bool MemorySystem::detect_conflict(const std::string& fact,
                                   const std::string& embedding_json) const {
    if (!query_cb_ || !local_llm_cb_) return false;

    nlohmann::json where = {{"state", {{"$ne", "dormant"}}}};
    auto sim_str = query_cb_(facts_collection_, embedding_json, 3, where.dump());
    auto sim = nlohmann::json::parse(sim_str);

    auto sim_ids = sim.value("ids", nlohmann::json::array());
    if (sim_ids.empty() || sim_ids[0].empty()) return false;

    nlohmann::json existing_facts = nlohmann::json::array();
    for (size_t i = 0; i < sim_ids[0].size(); ++i) {
        float similarity = 1.0f - sim["distances"][0][i].get<float>();
        if (similarity < CONFLICT_SIM_THRESHOLD) continue;
        existing_facts.push_back(sim["documents"][0][i].get<std::string>());
    }

    if (existing_facts.empty()) {
        std::cerr << "  [conflict] \"" << fact << "\" -- no similar facts (skipped)\n";
        return false;
    }

    std::string numbered;
    for (size_t i = 0; i < existing_facts.size(); ++i)
        numbered += std::to_string(i + 1) + ". \"" + existing_facts[i].get<std::string>() + "\"\n";

    std::cerr << "  [conflict] Checking \"" << fact << "\" against "
              << existing_facts.size() << " similar:\n";
    for (size_t i = 0; i < existing_facts.size(); ++i)
        std::cerr << "    " << (i+1) << ". \"" << existing_facts[i].get<std::string>() << "\"\n";

    std::string prompt =
        "You are checking narrative facts for a text RPG.\n"
        "A new fact is being added. Check if it contradicts any existing facts.\n"
        "Consider BOTH direct and transitive contradictions "
        "(e.g. if A=brother(B) and B=brother(C), then A must be brother(C), not cousin).\n\n"
        "Existing facts:\n" + numbered + "\n"
        "New fact: \"" + fact + "\"\n\n"
        "Respond as JSON: {\"conflicts\": true/false, \"reason\": \"...\"}";

    auto raw = local_llm_cb_(prompt);
    if (raw.empty()) {
        std::cerr << "  [conflict] LLM returned empty, assuming no conflict\n";
        return false;
    }

    try {
        auto result = nlohmann::json::parse(raw);
        bool conflicts = result.value("conflicts", false);
        std::string reason = result.value("reason", "(none)");
        if (conflicts) {
            std::cerr << "  [conflict] >>> CONFLICT -- DISCARDED <<<\n"
                      << "    Reason: " << reason << "\n" << std::flush;
        } else {
            std::cerr << "  [conflict] OK -- " << reason << "\n";
        }
        return conflicts;
    } catch (const std::exception& e) {
        std::cerr << "  [conflict] PARSE FAILED: " << e.what() << "\n"
                  << "  [conflict] Raw was: " << raw.substr(0, 300) << "\n" << std::flush;
        return false;
    }
}

void MemorySystem::sync_resolved(const std::vector<Node>& resolved_nodes, int turn) {
    if (!get_by_meta_cb_ || !update_meta_cb_) return;

    int synced = 0;
    for (const auto& node : resolved_nodes) {
        auto hash = md5_hash(node.fact);
        nlohmann::json where = {{"hash", hash}};
        auto result_json = get_by_meta_cb_(facts_collection_, where.dump());
        auto result = nlohmann::json::parse(result_json);

        auto ids = result.value("ids", nlohmann::json::array());
        if (ids.empty()) continue;

        for (const auto& id : ids) {
            nlohmann::json meta = {
                {"state", "resolved"},
                {"resolved_at", turn},
            };
            update_meta_cb_(facts_collection_, id.get<std::string>(), meta.dump());
            synced++;
        }
    }

    if (synced > 0)
        std::cerr << "  [memory] Synced " << synced << " resolved facts to ChromaDB\n" << std::flush;
}

// -- Full Pipeline --

void MemorySystem::process_new_nodes(const std::vector<Node>& nodes, int turn) {
    if (nodes.empty()) return;

    nlohmann::json nodes_json = nlohmann::json::array();
    for (const auto& n : nodes) {
        nodes_json.push_back({
            {"fact",     n.fact},
            {"state",    to_string(n.state)},
            {"type",     n.type},
            {"known_by", n.known_by},
            {"related_to", n.related_to},
        });
    }

    std::cerr << "\n  --- Memory pipeline: " << nodes.size() << " incoming nodes ---\n";

    // 1. Distill verbose facts
    nlohmann::json distilled = nlohmann::json::array();
    for (const auto& node : nodes_json) {
        std::string orig = node.value("fact", "");
        auto parts = distill_verbose(orig);
        if (parts.size() > 1 || (parts.size() == 1 && parts[0] != orig)) {
            std::cerr << "  [distill] \"" << orig << "\" -> ";
            for (size_t i = 0; i < parts.size(); ++i)
                std::cerr << (i ? " | " : "") << "\"" << parts[i] << "\"";
            std::cerr << "\n";
        }
        for (const auto& fact : parts) {
            auto copy = node;
            copy["fact"] = fact;
            distilled.push_back(std::move(copy));
        }
    }

    // 2. Quality gate
    std::cerr << "  [quality] Scoring " << distilled.size() << " facts...\n";
    auto pool_json = retrieve(nodes_json[0].value("fact", ""), 15);
    auto accepted_str = score_quality_batch(distilled.dump(), pool_json);

    // 3. Entity extraction (single batched LLM call)
    auto with_entities_str = extract_entities_batch(accepted_str);
    auto with_entities = nlohmann::json::parse(with_entities_str);

    std::cerr << "  [entities] Results:\n";
    for (const auto& node : with_entities) {
        std::cerr << "    \"" << node.value("fact", "") << "\" -> [";
        auto ents = node.value("entities", nlohmann::json::array());
        for (size_t i = 0; i < ents.size(); ++i)
            std::cerr << (i ? ", " : "") << ents[i].get<std::string>();
        std::cerr << "]\n";
    }

    // 4. Embed once per fact, conflict detection, then store
    std::cerr << "  [store] Embedding + conflict check + store...\n";
    int stored = 0, conflicts = 0, dupes = 0;
    for (const auto& node : with_entities) {
        std::string fact = node.value("fact", "");
        std::vector<std::string> entities;
        for (const auto& e : node.value("entities", nlohmann::json::array()))
            entities.push_back(e.get<std::string>());

        std::vector<std::string> known_by;
        for (const auto& k : node.value("known_by", nlohmann::json::array()))
            known_by.push_back(k.get<std::string>());
        std::vector<std::uint64_t> related_to;
        for (const auto& rid : node.value("related_to", nlohmann::json::array()))
            related_to.push_back(rid.get<std::uint64_t>());

        auto embedding_json = embed_cb_(fact);

        if (detect_conflict(fact, embedding_json)) {
            conflicts++;
            continue;
        }

        auto id = store_fact(fact, embedding_json, node.value("state", "active"),
                             node.value("type", "plot"), known_by, entities, related_to, turn);
        if (id.empty()) {
            std::cerr << "  [memory] DUPLICATE: \"" << fact << "\"\n";
            dupes++;
        } else {
            stored++;
        }
    }
    std::cerr << "  [memory] Done: " << stored << " stored, "
              << conflicts << " conflicts, " << dupes << " duplicates\n" << std::flush;
}

} // namespace rhapsode
