#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "rhapsode/character.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

using ReflectionLLMCallback = std::function<std::string(const std::string& prompt)>;



class CharacterMemory {
public:
    enum MemoryType {
        kEvent = 0, 
        kThought = 1, 
        kChat = 2, 
        kUndefined = 3
    }; 
    struct MemoryNode {
        std::uint64_t id    = 0;
        MemoryType type     = MemoryType::kUndefined;
        std::string content = "";
        std::int64_t created_at  = 0;
        float weight        = .0f; 
        short depth         = 0; 
        short last_accessed = 0;  // updated by retrieve(), used for recency scoring
    };
    using MemGraph = boost::adjacency_list<
        boost::vecS, boost::vecS, boost::directedS, 
        MemoryNode, EdgeData
    >;

    explicit CharacterMemory(std::string name);

    // Record the character's spoken dialogue as a kChat memory.
    // Scores importance, drains reflection_countdown_.
    void speak(const std::string& scene_context, int turn); 
    
    // Record what the character perceived as a kEvent memory.
    // Scores importance, drains reflection_countdown_.
    void observe(const std::string& scene_context, int turn);
    
    // If reflection_countdown_ <= 0, run the full GA reflection pipeline:
    //   1. Generate focal questions from recent memories (LLM)
    //   2. For each question, retrieve evidence via retrieve()
    //   3. Generate insights from evidence (LLM), dedup against existing
    //   4. Store surviving insights as kThought memories with depth > 0
    //   5. Link each insight to its evidence nodes via add_edge()
    //   6. Reset reflection_countdown_ to kReflectionInterval
    // Early return if countdown > 0 or no LLM callback.
    void reflect(); 

    // True when the countdown has been drained and an LLM callback is available.
    bool needs_reflection() const;

    // Return top-k memories scored by recency + relevance + importance.
    // Updates last_accessed on returned nodes (recency signal for future calls).
    // Used by speak() to pull context, and by reflect() to gather evidence.
    std::string retrieve(const std::string& query, int top_k = 5);

    // Synthesize retrieve() output into a coherent 3-5 sentence briefing
    // via local LLM. Falls back to raw retrieve() on any failure.
    std::string briefing(const std::string& query, int top_k = 5);

    // -- Persistent first-person self-state (running "who I am right now") --
    // The carried inner monologue, advanced each turn by update_self_state().
    // Unlike retrieve()/briefing() it is NOT re-derived from a query: it folds
    // the previous state forward, so an emotional thread persists across topic
    // shifts until it is genuinely resolved.
    const std::string& self_state() const { return self_state_; }

    // Set the self-state directly. Used to seed from authored interiority
    // (initial_memory.context) at scenario load, before any LLM is available.
    void set_self_state(std::string s) { self_state_ = std::move(s); }

    // Set the character's persona (their Character.description) -- the source of
    // identity and pronouns for first-person prompts.  Not serialized; the Scene
    // re-attaches it from Character.description on every load (single source of
    // truth), so prompts never have to guess gender/role.
    void set_persona(std::string p) { persona_ = std::move(p); }

    // Fold the previous self_state_ together with what just happened into an
    // updated first-person inner state (local LLM). No-op (keeps prior state)
    // if no reflection LLM callback is set or the completion is empty.
    void update_self_state(int turn);

    // Ask the LLM to rate a description's importance to this character (1-10).
    // Called by observe() and speak() to set weight on new memories,
    // and by reflect() to score new insights.
    int score_importance(const std::string& description) const;

    // -- Callbacks (set from Python, once at session start) --
    void set_embed_callback(EmbedCallback cb);      // text -> embedding vector (JSON string)
    void set_store_callback(StoreCallback cb);       // upsert doc + embedding + metadata to ChromaDB
    void set_query_callback(QueryCallback cb);       // query ChromaDB by embedding, return results
    void set_reflection_llm_callback(ReflectionLLMCallback cb); // prompt -> completion (local LLM)

    // Insert a kEvent seed with weight=0 (no LLM call). Used at bootstrap
    // to populate the memory stream from WorldGraph facts before callbacks are set.
    void seed_from_graph(const std::string& fact, int created_at);

    // -- Subjective belief graph (beliefs_) --
    // A character's view of the world and of others is a WorldGraph of its own,
    // owned subjectively.  Authored priors are seeded here; perception is routed
    // here (later slices); the actor speaks only from here -- never the narrator's
    // omniscient graph.

    // Seed an authored belief as an Active node tagged with its subject entities.
    void seed_belief(const std::string& fact,
                     const std::vector<std::string>& entities,
                     int created_at);

    // What I currently believe about the given subjects (character names and/or
    // descriptions): my live Thoughts about them rendered as oldest->newest
    // chains (graph adjacency), ordered by how much they press (weight) with
    // contradictions kept live as tension cross-links.  Empty if I hold no view.
    std::string view_of(const std::vector<std::string>& subjects) const;

    // Route a perceived fact into this mind (perception layer).  Stored as an
    // Active "perception" node tagged with its subject entities, awaiting
    // reflect_perceptions().  No LLM.  This is the ONLY channel by which a mind
    // learns of the world after bootstrap -- the narrator decides who perceives.
    void route_fact(const std::string& fact,
                    const std::vector<std::string>& entities,
                    int turn);

    // Consolidate unreflected perceptions into beliefs (interpretation layer).
    // For each subject entity, fold (my prior belief + persona + new perceptions)
    // into an updated first-person belief (local LLM), superseding the prior
    // belief via valid_until (the old one is preserved as history, not deleted).
    // No-op without a reflection LLM callback or when nothing new was perceived.
    void reflect_perceptions(int turn);

    const WorldGraph& beliefs() const { return beliefs_; }

    // Render my live Thoughts as oldest->newest chains by traversing belief-graph
    // adjacency (NOT Node.related_to): grouped by subject, each chain annotated
    // with its peak weight, contradictions drawn as tension cross-links.  When
    // `subjects` is empty, render every chain (dispositional self-view included);
    // otherwise restrict to chains about a matching subject.  Chains are ordered
    // most-pressing first.  Used by view_of and the narrator inner-state context.
    std::string render_thoughts(const std::vector<std::string>& subjects = {}) const;

    // -- Gated experiments (rendered as context for the narrator, never a
    //    command).  See the "Phase 3 experiments" section of the design. --

    // Among my most-charged live Thoughts, pick one (seeded) as the one
    // "pressing today".  Returns its text, or "" if nothing presses.
    std::string pressing_thought(unsigned seed) const;

    // If a Thought sits near the ceiling, name its charge: an unbearable tension
    // (it is cross-linked) or a hardened conviction (it stands alone).  "" else.
    std::string charge_state() const;

    // Re-embed and upsert all memories to ChromaDB. Called once at session
    // start to ensure ChromaDB is in sync with the graph after loading a save.
    void sync_to_chroma();

    // -- Serialization --
    nlohmann::json to_json() const;                  // graph + edges + countdown -> JSON
    static CharacterMemory from_json(const nlohmann::json& j); // JSON -> CharacterMemory

    const std::string& name() const { return char_name_; }

private:
    std::string char_name_;

    // Persistent first-person inner state, carried across turns (see above).
    std::string self_state_;

    // The character's description, used to ground first-person prompts in the
    // right identity/pronouns.  Set via set_persona(); not serialized.
    std::string persona_;

    // Preamble line injected into first-person prompts ("" when no persona).
    std::string persona_line() const {
        return persona_.empty() ? std::string() : "Who I am: " + persona_ + "\n";
    }

    static constexpr int64_t kReflectionInterval = 60;
    int64_t reflection_countdown_ = kReflectionInterval;

    // The subjective belief graph -- same structure as the narrator's WorldGraph,
    // owned per-character.  This is what the actor reads from.
    WorldGraph beliefs_;

    MemGraph graph_;
    int next_node_id_ = 1;

    MemoryNode& add(MemoryNode node);
    MemoryNode* get(std::uint64_t id);
    bool add_edge(std::uint64_t from_id, std::uint64_t to_id, EdgeData& edge_payload);

    // Compress long text into a concise memory via local LLM.
    // Short text (<= 200 chars) passes through unchanged.
    std::string distill(const std::string& text) const;

    EmbedCallback embed_cb_;
    StoreCallback store_cb_;
    QueryCallback query_cb_;
    ReflectionLLMCallback reflection_llm_cb_;
};

} // namespace rhapsode
