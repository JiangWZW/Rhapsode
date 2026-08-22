#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace rhapsode {

// These records are inert until a later phase explicitly selects and commits
// them. Phase 4 only populates them from the legacy narrator response.
struct ActorProposal {
    std::string proposal_id;
    std::string character_id;
    std::string action;
    std::string exact_dialogue;
    std::vector<std::string> evidence_refs;
    std::optional<std::uint64_t> character_core_version;
    std::uint64_t base_state_version = 0;
    std::string source;
};

enum class MechanicalOperationKind {
    EnsureSceneMember,
    CreateCharacter,
};

struct MechanicalOperation {
    MechanicalOperationKind kind = MechanicalOperationKind::EnsureSceneMember;
    std::string scene_id;
    std::string character_id;
    nlohmann::json arguments = nlohmann::json::object();
};

struct TurnDecision {
    std::string decision_id;
    std::uint64_t base_state_version = 0;
    std::vector<std::string> accepted_proposal_ids;
    std::vector<MechanicalOperation> mechanical_operations;
    std::vector<std::string> canonical_events;
    std::vector<std::string> evidence_refs;
    std::string source;
};

struct ShadowAuditIssue {
    std::string code;
    std::string detail;
};

struct LegacyTurnShadow {
    std::string source = "legacy_narrator";
    std::vector<ActorProposal> proposals;
    TurnDecision decision;
    // Graph extraction still follows prose on the legacy path. Preserve its
    // exact plan for comparison without promoting it to mechanics or events.
    nlohmann::json observation_graph_plan = nlohmann::json::object();
    std::vector<ShadowAuditIssue> issues;
};

}  // namespace rhapsode
