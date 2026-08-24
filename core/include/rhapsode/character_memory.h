#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/llm_callback.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

class World;

struct CharacterCore {
    std::string text;   // continuity sheet — NOT a thought stream
    int revised_at = -1;
};

struct MonologueLine {
    int turn = 0;
    std::string text;
};

class CharacterMemory {
public:
    static constexpr float kAuthoredSeedWeight = 4.0f;
    static constexpr int kPerceptionWindowTurns = 3;
    static constexpr std::size_t kPerceptionUserChars = 1800;

    explicit CharacterMemory(std::string name);

    std::uint64_t seed_belief(const std::string& fact,
                              const std::vector<std::string>& entities,
                              int created_at,
                              float weight = kAuthoredSeedWeight,
                              const std::string& type = "belief");

    bool expire_intention(std::uint64_t node_id, int valid_until);
    void link_tension(std::uint64_t a_id, std::uint64_t b_id, int turn);

    // Fill core text when it is still empty.
    void ensure_bootstrap(const std::string& core_text_if_empty);

    const WorldGraph &beliefs() const { return beliefs_; }
    const CharacterCore &core() const { return core_; }
    const std::string& perception() const { return perception_; }
    int perception_turn() const { return perception_turn_; }
    int monologue_turn() const { return monologue_turn_; }

    std::string build_perception_prompt(const std::string& narration_window,
                                        const std::string& who = {});
    void apply_perception_json(int turn, const std::string& raw);
    void update_perception(int turn, const std::string& who,
                           const std::string& narration_window,
                           const LLMCallback& llm_callback);

    void update_monologues(int turn,
                           const std::string& description,
                           const LLMCallback& llm_callback);
    std::string build_monologue_prompt(const std::string& description);
    void apply_monologue_json(int turn, const std::string& raw);

    const std::vector<MonologueLine>& monologue_lines() const {
        return monologue_lines_;
    }

    std::string render_thoughts(const std::vector<std::string>& subjects = {}) const;

    // Compact mind payload for query_mind: core + perception + recent monologue + beliefs.
    nlohmann::json render_mind_query(std::size_t max_belief_chars = 1200,
                                     std::size_t max_line_chars = 400) const;

    nlohmann::json to_json() const;
    static CharacterMemory from_json(const nlohmann::json& j);

    const std::string& name() const { return character_name_; }

    friend class World;

private:
    static constexpr int kStagingBuffers = 3;

    int claim_perception(int turn);
    void release_perception(int i);
    bool perception_pending() const;
    bool perception_pending(int i) const { return perception_pending_[i]; }
    int perception_claim_turn(int i) const { return perception_claim_turn_[i]; }

    int claim_monologue(int turn);
    void release_monologue(int i);
    bool monologue_pending() const;
    bool monologue_pending(int i) const { return monologue_pending_[i]; }
    int monologue_claim_turn(int i) const { return monologue_claim_turn_[i]; }

    std::string character_name_;
    WorldGraph beliefs_;
    CharacterCore core_;
    std::string perception_;
    int perception_turn_ = -1;
    int monologue_turn_ = -1;

    std::vector<MonologueLine> monologue_lines_;
    int monologue_claim_turn_[kStagingBuffers] = {-1, -1, -1};
    bool monologue_pending_[kStagingBuffers] = {};
    int monologue_write_ = 0;

    int perception_claim_turn_[kStagingBuffers] = {-1, -1, -1};
    bool perception_pending_[kStagingBuffers] = {};
    int perception_write_ = 0;
};

}  // namespace rhapsode
