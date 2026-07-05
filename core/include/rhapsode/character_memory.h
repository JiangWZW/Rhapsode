#pragma once

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {


class CharacterMemory {
public:
    // Default starting charge for an authored (seeded) belief: high enough that
    // authored priors surface in render_thoughts and sit well above the cull
    // floor until the character genuinely stops revisiting them.
    static constexpr float kAuthoredSeedWeight = 4.0f;

    explicit CharacterMemory(std::string name);

    // -- Callback (set from Python, once at session start) --
    void set_reflection_llm_callback(LLMCallback cb); // prompt -> completion (cloud LLM)

    // -- Subjective belief graph (beliefs_) --
    // A character's view of the world and of others is a WorldGraph of its own,
    // owned subjectively.  Authored priors are seeded here; perception is routed
    // here; the narrator writes FROM here -- never from the omniscient graph.

    // Seed an authored belief as an Active node tagged with its subject entities.
    // `weight` is its starting charge (how much it presses on the character).
    // Returns the new node id so callers can cross-link authored contradictions.
    std::uint64_t seed_belief(const std::string& fact,
                              const std::vector<std::string>& entities,
                              int created_at,
                              float weight = kAuthoredSeedWeight);

    // Cross-link two of my Thoughts as a held contradiction (a "tension" edge),
    // so the pair surfaces in the rendered Tensions section.  Both stay live.
    // No-op if either node id is unknown.
    void link_tension(std::uint64_t a_id, std::uint64_t b_id, int turn);

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
    // One batched cloud LLM call per character folds (prior beliefs + description +
    // new perceptions) into new first-person Thoughts, typed tension/evidence
    // against priors, then reinforces touched neighbors and decays/culls the
    // untouched.  No-op without a reflection LLM callback or new perceptions.
    // `description` is the character's Character.description, passed in so the
    // memory does not duplicate it.
    void reflect_perceptions(int turn, const std::string& description);

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

    // -- Serialization --
    nlohmann::json to_json() const;                  // belief graph -> JSON
    static CharacterMemory from_json(const nlohmann::json& j); // JSON -> CharacterMemory

    const std::string& name() const { return char_name_; }

private:
    std::string char_name_;

    // The subjective belief graph -- same structure as the narrator's WorldGraph,
    // owned per-character.  This is the whole persisted mind.
    WorldGraph beliefs_;

    LLMCallback reflection_llm_cb_;
};

} // namespace rhapsode
