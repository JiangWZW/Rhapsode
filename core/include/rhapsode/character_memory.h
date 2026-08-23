#pragma once

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
    int seq = 0;
};

struct MonologueStream {
    std::string id;
    std::string focus;
    std::string status = "active";  // active | closed
    std::vector<MonologueLine> lines;
    std::string parent_id;
    std::string closed_reason;
    std::string closed_summary;
};

struct Perception {
    std::uint64_t id = 0;
    std::string fact;
    std::vector<std::string> entities;
};

struct ObjectiveLine {
    int turn = 0;
    std::string type;  // take | seen
    std::string text;
};


class CharacterMemory {
public:
    static constexpr float kAuthoredSeedWeight = 4.0f;
    static constexpr int kMaxActiveStreams = 5;

    explicit CharacterMemory(std::string name);

    std::uint64_t seed_belief(const std::string& fact,
                              const std::vector<std::string>& entities,
                              int created_at,
                              float weight = kAuthoredSeedWeight,
                              const std::string& type = "belief");

    bool expire_intention(std::uint64_t node_id, int valid_until);
    void link_tension(std::uint64_t a_id, std::uint64_t b_id, int turn);

    void append_objective(int turn, std::string kind, std::string text);

    // Ensure core text (if empty) and at least one active bootstrap stream.
    void ensure_bootstrap(const std::string& core_text_if_empty);

    // Append seen lines (and optional facts) from this mind's journal + latest take.
    void update_objective_journal(int turn,
                                  const std::string& who,
                                  const LLMCallback& llm_callback);
    std::string build_observation_prompt() const; 
    void apply_observation_json(int turn, const std::string &raw);

    const WorldGraph &beliefs() const { return beliefs_; }
    const CharacterCore &core() const { return core_; }

    // On-stage actor update: streams + optional knows[] + rare core_revision.
    // Expires leftover Active perception nodes from old saves. No reflect LLM.
    // voice is unused in the prompt blob; kept on the signature.
    struct MonologueBuildContext {
        std::string prompt;
        std::vector<Perception> perceptions;
        std::string description;
    };
    void update_monologues(int turn,
                           const std::string& description,
                           const std::string& turn_stimulus,
                           const LLMCallback& llm_callback,
                           const std::string& voice = {});
    MonologueBuildContext build_monologue_prompt(const std::string &description,
                                                           const std::string &turn_stimulus);
    void apply_monologue_json(int turn, const std::string &raw, const MonologueBuildContext &ctx);

    int monologue_consumed_lines() const { return monologue_consumed_lines_; }
    void set_monologue_consumed_lines(int n) { monologue_consumed_lines_ = n; }
    const std::vector<MonologueStream>& streams() const { return streams_; }
    int active_stream_count() const;

    const std::vector<ObjectiveLine>& objective_journal() const { return objective_journal_; }
    int observation_consumed_lines() const { return objective_journal_consumed_lines_; }
    void set_observation_consumed_lines(int n) { objective_journal_consumed_lines_ = n; }


    std::string render_thoughts(const std::vector<std::string>& subjects = {}) const;

    // Compact mind payload for query_mind: core + streams + beliefs.
    nlohmann::json render_mind_query(std::size_t max_belief_chars = 1200,
                                     std::size_t max_line_chars = 400) const;

    nlohmann::json to_json() const;
    static CharacterMemory from_json(const nlohmann::json& j);

    const std::string& name() const { return character_name_; }

    friend class World;

private:
    static constexpr int kStagingBuffers = 3;

    int claim_observation(int num_lines);
    void release_observation(int i);
    bool observation_pending() const;
    bool observation_pending(int i) const { return observation_pending_[i]; }
    int observation_num_lines(int i) const { return observation_num_lines_[i]; }

    int claim_monologue(int num_lines, MonologueBuildContext ctx);
    void release_monologue(int i);
    bool monologue_pending() const;
    bool monologue_pending(int i) const { return monologue_pending_[i]; }
    int monologue_num_lines(int i) const { return monologue_num_lines_[i]; }
    const MonologueBuildContext& monologue_context(int i) const {
        return monologue_ctx_[i];
    }

    std::string character_name_;
    WorldGraph beliefs_;
    CharacterCore core_;

    std::vector<MonologueStream> streams_;
    int monologue_stream_cnt_ = 0;
    int monologue_consumed_lines_ = 0;
    int monologue_num_lines_[kStagingBuffers] = {-1, -1, -1};
    bool monologue_pending_[kStagingBuffers] = {};
    MonologueBuildContext monologue_ctx_[kStagingBuffers] = {};
    int monologue_write_ = 0;
    MonologueStream *find_stream(const std::string &id);
    std::string alloc_stream_id(const std::string &parent_id, int turn);
    void append_monologue_line(MonologueStream &stream, int turn, std::string text);

    std::vector<ObjectiveLine> objective_journal_;
    int objective_journal_line_cnt_ = 0;
    int objective_journal_consumed_lines_ = 0;
    int observation_num_lines_[kStagingBuffers] = {-1, -1, -1};
    bool observation_pending_[kStagingBuffers] = {};
    int observation_write_ = 0;
};

}  // namespace rhapsode
