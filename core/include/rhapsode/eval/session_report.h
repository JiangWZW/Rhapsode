#pragma once

#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"

namespace rhapsode {

enum class EndReason {
    MaxTurns,
    TurnTimeout,
    ServerExit,
    WsError,
    TurnError,
};

std::string end_reason_name(EndReason reason);
EndReason end_reason_from_name(const std::string& name);

struct ReliabilityMetrics {
    int turns_completed = 0;
    int turns_requested = 0;
    std::vector<double> turn_ms;
    int timeouts = 0;
    int errors = 0;
    int server_exit_code = 0;
    std::vector<std::string> log_markers;
};

struct NarrativeMetrics {
    int empty_beats = 0;
    double repetition_score = 0.0;
    double length_collapse = 0.0;
    int cast_gaps = 0;
    std::vector<std::string> findings;
};

struct SessionReport {
    EndReason end_reason = EndReason::MaxTurns;
    ReliabilityMetrics reliability;
    NarrativeMetrics narrative;
    std::string critique;

    static SessionReport from_run_dir(const std::string& dir,
                                      LLMCallback critique_llm = {});
    void write(const std::string& dir) const;
};

}  // namespace rhapsode
