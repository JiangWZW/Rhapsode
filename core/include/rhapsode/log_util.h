#pragma once

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace rhapsode {

enum class LogLevel { Error = 0, Warn = 1, Info = 2, Debug = 3 };

struct LogContext {
    std::string scene;
    int turn = -1;
    std::string kind;  // "player" | "offstage" | ""
};

inline LogContext& log_context() {
    thread_local LogContext ctx;
    return ctx;
}

inline void set_log_context(std::string scene, int turn, std::string kind) {
    auto& ctx = log_context();
    ctx.scene = std::move(scene);
    ctx.turn = turn;
    ctx.kind = std::move(kind);
}

inline void clear_log_context() {
    log_context() = {};
}

inline bool verbose_logging_enabled() {
    return std::getenv("RHAPSODE_VERBOSE_LOG") != nullptr;
}

inline LogLevel configured_log_level() {
    if (verbose_logging_enabled()) return LogLevel::Debug;
    const char* env = std::getenv("RHAPSODE_LOG_LEVEL");
    if (!env || !*env) return LogLevel::Info;
    if (std::strcmp(env, "DEBUG") == 0 || std::strcmp(env, "debug") == 0)
        return LogLevel::Debug;
    if (std::strcmp(env, "WARNING") == 0 || std::strcmp(env, "WARN") == 0 ||
        std::strcmp(env, "warning") == 0 || std::strcmp(env, "warn") == 0)
        return LogLevel::Warn;
    if (std::strcmp(env, "ERROR") == 0 || std::strcmp(env, "error") == 0)
        return LogLevel::Error;
    return LogLevel::Info;
}

inline bool log_enabled(LogLevel level) {
    return static_cast<int>(level) <= static_cast<int>(configured_log_level());
}

namespace detail {

inline const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::Error: return "ERROR";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Debug: return "DEBUG";
    }
    return "INFO ";
}

class NullBuf : public std::streambuf {
protected:
    int overflow(int ch) override { return ch; }
};

inline std::ostream& null_stream() {
    static NullBuf buf;
    static std::ostream os(&buf);
    return os;
}

inline void write_prefix(LogLevel level, const char* component) {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    std::cerr << std::put_time(&local, "%H:%M:%S") << ' '
              << level_name(level) << ' ' << component << ": ";
}

}  // namespace detail

inline std::ostream& log_at(LogLevel level, const char* component) {
    if (!log_enabled(level)) return detail::null_stream();
    detail::write_prefix(level, component);
    return std::cerr;
}

inline std::ostream& log_error(const char* component) {
    auto& os = log_at(LogLevel::Error, component);
    return os;
}

inline std::ostream& log_warn(const char* component) {
    return log_at(LogLevel::Warn, component);
}

inline std::ostream& log_info(const char* component) {
    return log_at(LogLevel::Info, component);
}

inline std::ostream& log_debug(const char* component) {
    return log_at(LogLevel::Debug, component);
}

/// Legacy sink — treated as DEBUG so quiet INFO consoles stay clean until
/// call sites are migrated to log_info / log_debug / log_error.
inline std::ostream& log() {
    return log_debug("app");
}

}  // namespace rhapsode
