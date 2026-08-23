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

struct ObjectiveLine {
    int turn = 0;
    std::string kind;  // take | seen
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

    // On-stage actor update: streams + optional knows[] + rare core_revision.
    // Expires leftover Active perception nodes from old saves. No reflect LLM.
    // voice is unused in the prompt blob; kept on the signature.
    void update_monologues(int turn,
                           const std::string& description,
                           const std::string& turn_stimulus,
                           const LLMCallback& llm_callback,
                           const std::string& voice = {});

    const WorldGraph& beliefs() const { return beliefs_; }
    const CharacterCore& core() const { return core_; }
    const std::vector<MonologueStream>& streams() const { return streams_; }
    const std::vector<ObjectiveLine>& objective_journal() const {
        return objective_journal_;
    }

    int active_stream_count() const;

    std::string render_thoughts(const std::vector<std::string>& subjects = {}) const;

    // Compact mind payload for query_mind: core + streams + beliefs.
    nlohmann::json render_mind_query(std::size_t max_belief_chars = 1200,
                                     std::size_t max_line_chars = 400) const;

    nlohmann::json to_json() const;
    static CharacterMemory from_json(const nlohmann::json& j);

    const std::string& name() const { return character_name_; }

private:
    MonologueStream* find_stream(const std::string& id);
    std::string alloc_stream_id(const std::string& parent_id, int turn);
    void append_line(MonologueStream& stream, int turn, std::string text);

    std::string character_name_;
    WorldGraph beliefs_;
    CharacterCore core_;
    std::vector<MonologueStream> streams_;
    std::vector<ObjectiveLine> objective_journal_;
    int stream_seq_ = 0;
    int line_seq_ = 0;
};

}  // namespace rhapsode
