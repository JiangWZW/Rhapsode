#include "rhapsode/eval/session_eval.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#endif

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "rhapsode/log_util.h"

namespace rhapsode {
namespace {

namespace fs = std::filesystem;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

struct WsMessage {
    std::string type;
    json data;
};

struct TurnExchange {
    std::vector<WsMessage> messages;
    bool timed_out = false;
    bool ws_failed = false;
    bool turn_error = false;
    std::string error_detail;
};

#if defined(_WIN32)
class ServerProcess {
public:
    ServerProcess() = default;
    ~ServerProcess() { stop(); }

    ServerProcess(const ServerProcess&) = delete;
    ServerProcess& operator=(const ServerProcess&) = delete;

    bool spawn(const std::string& cmd, const fs::path& log_path) {
        stop();
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        HANDLE log_handle = CreateFileW(
            log_path.wstring().c_str(), FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (log_handle == INVALID_HANDLE_VALUE) return false;

        // Kill-on-close job so cmd.exe → python → uvicorn children die with us.
        HANDLE job = CreateJobObjectW(nullptr, nullptr);
        if (!job) {
            CloseHandle(log_handle);
            return false;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                     &limits, sizeof(limits))) {
            CloseHandle(job);
            CloseHandle(log_handle);
            return false;
        }

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = log_handle;
        si.hStdError = log_handle;

        std::wstring wcmd(cmd.begin(), cmd.end());
        PROCESS_INFORMATION pi{};
        const BOOL ok = CreateProcessW(
            nullptr, wcmd.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi);
        CloseHandle(log_handle);
        if (!ok) {
            CloseHandle(job);
            return false;
        }
        if (!AssignProcessToJobObject(job, pi.hProcess)) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            CloseHandle(job);
            return false;
        }
        ResumeThread(pi.hThread);
        CloseHandle(pi.hThread);
        process_ = pi.hProcess;
        job_ = job;
        pid_ = pi.dwProcessId;
        return true;
    }

    bool alive() const {
        if (!process_) return false;
        return WaitForSingleObject(process_, 0) == WAIT_TIMEOUT;
    }

    int exit_code() const {
        if (!process_) return 0;
        DWORD code = 0;
        if (GetExitCodeProcess(process_, &code)) return static_cast<int>(code);
        return -1;
    }

    void stop() {
        // Closing the job kills the whole process tree (cmd + uvicorn children).
        if (job_) {
            CloseHandle(job_);
            job_ = nullptr;
        }
        if (process_) {
            if (alive()) TerminateProcess(process_, 1);
            WaitForSingleObject(process_, 5000);
            CloseHandle(process_);
            process_ = nullptr;
        }
        pid_ = 0;
    }

private:
    HANDLE process_ = nullptr;
    HANDLE job_ = nullptr;
    DWORD pid_ = 0;
};
#else
class ServerProcess {
public:
    bool spawn(const std::string&, const fs::path&) { return false; }
    bool alive() const { return false; }
    int exit_code() const { return -1; }
    void stop() {}
};
#endif

class WsClient {
public:
    void connect(const std::string& host, const std::string& port,
                 const std::string& path) {
        auto const results = resolver_.resolve(host, port);
        auto ep = beast::get_lowest_layer(ws_).connect(results);
        host_ = host + ":" + std::to_string(ep.port());
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(beast::http::field::user_agent, "rhapsode-session-eval");
            }));
        ws_.handshake(host_, path);
        ws_.text(true);
    }

    void send_player(const std::string& content) {
        json msg{{"type", "player_message"}, {"content", content}};
        ws_.write(net::buffer(msg.dump()));
    }

    TurnExchange wait_idle(int timeout_s) {
        TurnExchange ex;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(timeout_s);
        beast::flat_buffer buffer;
        while (true) {
            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                ex.timed_out = true;
                return ex;
            }
            const auto remain =
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
            beast::get_lowest_layer(ws_).expires_after(remain);
            beast::error_code ec;
            const auto bytes = ws_.read(buffer, ec);
            (void)bytes;
            if (ec) {
                ex.ws_failed = true;
                ex.error_detail = ec.message();
                return ex;
            }
            const std::string raw = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());
            json data = json::parse(raw, nullptr, false);
            if (!data.is_object()) continue;
            WsMessage msg;
            msg.type = data.value("type", "");
            msg.data = data;
            ex.messages.push_back(msg);
            if (msg.type == "error") {
                ex.turn_error = true;
                ex.error_detail = data.value("detail", "");
            }
            if (msg.type == "status" && data.value("state", "") == "idle")
                return ex;
        }
    }

    void close() {
        beast::error_code ec;
        ws_.close(websocket::close_code::normal, ec);
    }

private:
    net::io_context ioc_;
    tcp::resolver resolver_{ioc_};
    websocket::stream<beast::tcp_stream> ws_{ioc_};
    std::string host_;
};

std::string build_player_prompt(const std::vector<WsMessage>& recent) {
    std::ostringstream ss;
    ss << "Recent story:\n";
    int count = 0;
    for (auto it = recent.rbegin(); it != recent.rend() && count < 12; ++it) {
        if (it->type != "scene_message" && it->type != "user_message") continue;
        const std::string content = it->data.value("content", "");
        if (content.empty()) continue;
        ss << "- " << content << "\n";
        ++count;
    }
    if (count == 0) ss << "- (opening beat)\n";
    ss << "\nPlayer action:";
    return ss.str();
}

void append_turn_jsonl(const fs::path& path, const json& row) {
    std::ofstream out(path, std::ios::app);
    out << row.dump() << "\n";
}

void copy_saves(const fs::path& from, const fs::path& to) {
    if (from.empty() || !fs::exists(from)) return;
    std::error_code ec;
    fs::remove_all(to, ec);
    fs::create_directories(to.parent_path(), ec);
    fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
}

void write_manifest(const fs::path& out_dir, const SessionEvalConfig& config,
                    EndReason reason, int server_exit_code, int timeouts,
                    int errors, int turns_completed) {
    json manifest{
        {"end_reason", end_reason_name(reason)},
        {"max_turns", config.max_turns},
        {"turns_completed", turns_completed},
        {"server_exit_code", server_exit_code},
        {"timeouts", timeouts},
        {"errors", errors},
        {"ws_host", config.ws_host},
        {"ws_port", config.ws_port},
        {"ws_path", config.ws_path},
        {"saves_dir", config.saves_dir},
    };
    std::ofstream out(out_dir / "manifest.json");
    out << manifest.dump(2);
}

}  // namespace

SessionEvalRunner::SessionEvalRunner(SessionEvalConfig config)
    : config_(std::move(config)) {}

void SessionEvalRunner::set_player_llm(LLMCallback cb) {
    player_llm_ = std::move(cb);
}

void SessionEvalRunner::set_critique_llm(LLMCallback cb) {
    critique_llm_ = std::move(cb);
}

EndReason SessionEvalRunner::run() {
    if (!player_llm_)
        throw std::runtime_error("SessionEvalRunner: player LLM callback required");

    const fs::path out_dir(config_.out_dir);
    fs::create_directories(out_dir);
    const fs::path log_path = out_dir / "console.log";
    const fs::path turns_path = out_dir / "turns.jsonl";
    {
        std::ofstream(turns_path, std::ios::trunc);
    }

    ServerProcess server;
    bool spawned = false;
    if (!config_.server_cmd.empty()) {
        spawned = server.spawn(config_.server_cmd, log_path);
        if (!spawned)
            throw std::runtime_error("Failed to spawn server: " + config_.server_cmd);
        // Give uvicorn a moment to bind.
        for (int i = 0; i < 50; ++i) {
            if (!server.alive()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            try {
                WsClient probe;
                probe.connect(config_.ws_host, config_.ws_port, config_.ws_path);
                probe.close();
                break;
            } catch (...) {
                // keep waiting
            }
        }
    } else {
        std::ofstream(log_path, std::ios::trunc)
            << "[session_eval] no server_cmd; attaching to existing server\n"
            << "[session_eval] WARNING: server stdout is NOT captured here. "
               "Pass server_cmd so LLM timings land in console.log.\n";
    }

    EndReason reason = EndReason::MaxTurns;
    int timeouts = 0;
    int errors = 0;
    int turns_completed = 0;
    int server_exit_code = 0;
    std::vector<WsMessage> recent;

    try {
        WsClient ws;
        ws.connect(config_.ws_host, config_.ws_port, config_.ws_path);

        TurnExchange opening = ws.wait_idle(config_.open_timeout_s);
        recent.insert(recent.end(), opening.messages.begin(), opening.messages.end());
        if (spawned && !server.alive()) {
            reason = EndReason::ServerExit;
            server_exit_code = server.exit_code();
        } else if (opening.timed_out) {
            reason = EndReason::TurnTimeout;
            ++timeouts;
        } else if (opening.ws_failed) {
            reason = EndReason::WsError;
        } else if (opening.turn_error) {
            reason = EndReason::TurnError;
            ++errors;
        } else {
            for (int turn = 1; turn <= config_.max_turns; ++turn) {
                if (spawned && !server.alive()) {
                    reason = EndReason::ServerExit;
                    server_exit_code = server.exit_code();
                    break;
                }

                const std::string prompt = build_player_prompt(recent);
                std::string action;
                try {
                    action = player_llm_(prompt);
                } catch (const std::exception& ex) {
                    log() << "[session_eval] player LLM failed: " << ex.what() << "\n";
                    reason = EndReason::TurnError;
                    ++errors;
                    break;
                }
                // Trim whitespace.
                while (!action.empty() &&
                       (action.back() == '\n' || action.back() == '\r' ||
                        action.back() == ' '))
                    action.pop_back();
                if (action.empty()) action = "I look around carefully.";

                const auto t0 = std::chrono::steady_clock::now();
                ws.send_player(action);
                TurnExchange ex = ws.wait_idle(config_.turn_timeout_s);
                const auto t1 = std::chrono::steady_clock::now();
                const double t_ms =
                    std::chrono::duration<double, std::milli>(t1 - t0).count();

                recent.insert(recent.end(), ex.messages.begin(), ex.messages.end());
                json messages = json::array();
                for (const auto& m : ex.messages) messages.push_back(m.data);

                std::string status = "ok";
                if (ex.timed_out) {
                    status = "timeout";
                    reason = EndReason::TurnTimeout;
                    ++timeouts;
                } else if (ex.ws_failed) {
                    status = "ws_error";
                    reason = EndReason::WsError;
                } else if (ex.turn_error) {
                    status = "error";
                    reason = EndReason::TurnError;
                    ++errors;
                }

                append_turn_jsonl(turns_path, json{
                    {"turn", turn},
                    {"input", action},
                    {"t_ms", t_ms},
                    {"status", status},
                    {"error_detail", ex.error_detail},
                    {"messages", messages},
                });
                copy_saves(fs::path(config_.saves_dir), out_dir / "saves");
                ++turns_completed;

                if (status != "ok") break;
                if (spawned && !server.alive()) {
                    reason = EndReason::ServerExit;
                    server_exit_code = server.exit_code();
                    break;
                }
                if (turn == config_.max_turns) reason = EndReason::MaxTurns;
            }
        }
        ws.close();
    } catch (const std::exception& ex) {
        log() << "[session_eval] fatal: " << ex.what() << "\n";
        std::ofstream(log_path, std::ios::app) << "[session_eval] fatal: " << ex.what() << "\n";
        if (reason == EndReason::MaxTurns && turns_completed == 0)
            reason = EndReason::WsError;
    }

    if (spawned) {
        if (!server.alive()) server_exit_code = server.exit_code();
        server.stop();
    }

    write_manifest(out_dir, config_, reason, server_exit_code, timeouts, errors,
                   turns_completed);
    SessionReport report =
        SessionReport::from_run_dir(out_dir.string(), critique_llm_);
    report.end_reason = reason;
    report.write(out_dir.string());
    // Refresh manifest end_reason in case report overwrote nothing — already set.
    write_manifest(out_dir, config_, reason, server_exit_code, timeouts, errors,
                   turns_completed);
    return reason;
}

}  // namespace rhapsode
