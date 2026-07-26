#pragma once

#include <string>

#include "rhapsode/eval/session_report.h"
#include "rhapsode/llm_callback.h"

namespace rhapsode {

struct SessionEvalConfig {
    std::string ws_host = "127.0.0.1";
    std::string ws_port = "8080";
    std::string ws_path = "/ws";
    /// Empty = do not spawn; connect to an already-running server.
    std::string server_cmd;
    /// Server save directory to copy after each turn (e.g. server/saves).
    std::string saves_dir;
    std::string out_dir = "runs/latest";
    int max_turns = 5;
    int turn_timeout_s = 1200;
    /// Seconds to wait for the opening idle after connect.
    int open_timeout_s = 1200;
};

class SessionEvalRunner {
public:
    explicit SessionEvalRunner(SessionEvalConfig config);

    void set_player_llm(LLMCallback cb);
    void set_critique_llm(LLMCallback cb);

    /// Blocking: play, collect, analyze, write report under out_dir.
    EndReason run();

private:
    SessionEvalConfig config_;
    LLMCallback player_llm_;
    LLMCallback critique_llm_;
};

}  // namespace rhapsode
