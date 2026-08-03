#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/llm_callback.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

struct CharacterCore {
    std::string text;   // continuity sheet — NOT a thought stream
    int revised_at = -1;
};

struct MonologueLine {
    int turn = 0;
    std::string text;
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

    std::string view_of(const std::vector<std::string>& subjects) const;

    // Perception layer: stimulus for the next monologue update. Not auto-belief.
    void route_fact(const std::string& fact,
                    const std::vector<std::string>& entities,
                    int turn);

    // Ensure core text (if empty) and at least one active bootstrap stream.
    void ensure_bootstrap(const std::string& core_text_if_empty);

    // On-stage actor update: streams + optional knows[] + rare core_revision.
    // Consumes Active perceptions after the call. No separate reflect LLM.
    void update_monologues(int turn,
                           const std::string& description,
                           const std::string& beat_stimulus,
                           const LLMCallback& llm_callback);

    const WorldGraph& beliefs() const { return beliefs_; }
    const CharacterCore& core() const { return core_; }
    const std::vector<MonologueStream>& streams() const { return streams_; }

    int active_stream_count() const;

    std::string render_thoughts(const std::vector<std::string>& subjects = {}) const;

    // Compact mind payload for query_mind: core + streams + beliefs.
    nlohmann::json render_mind_query(std::size_t max_belief_chars = 1200,
                                     std::size_t max_line_chars = 400) const;

    std::string pressing_thought(unsigned seed) const;
    std::string charge_state() const;

    nlohmann::json to_json() const;
    static CharacterMemory from_json(const nlohmann::json& j);

    const std::string& name() const { return character_name_; }

private:
    MonologueStream* find_stream(const std::string& id);
    const MonologueStream* find_stream(const std::string& id) const;
    std::string alloc_stream_id(const std::string& parent_id, int turn);

    std::string character_name_;
    WorldGraph beliefs_;
    CharacterCore core_;
    std::vector<MonologueStream> streams_;
    int stream_seq_ = 0;
};

}  // namespace rhapsode
