#include "rhapsode/history.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace rhapsode {

static std::string utc_now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t_now);
#else
    gmtime_r(&time_t_now, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

void History::append(SceneMessage msg) {
    if (msg.timestamp.empty()) {
        msg.timestamp = utc_now_iso8601();
    }
    messages_.push_back(std::move(msg));
}

std::vector<SceneMessage> History::snapshot(std::optional<size_t> n) const {
    if (!n.has_value() || *n >= messages_.size()) {
        return messages_;
    }
    return std::vector<SceneMessage>(
        messages_.end() - static_cast<ptrdiff_t>(*n),
        messages_.end()
    );
}

size_t History::size() const {
    return messages_.size();
}

void History::truncate(size_t new_size) {
    if (new_size < messages_.size())
        messages_.resize(new_size);
}

void History::clear() {
    messages_.clear();
}

const std::vector<SceneMessage>& History::messages() const {
    return messages_;
}

void to_json(nlohmann::json& j, const History& h) {
    j = h.messages();
}

void from_json(const nlohmann::json& j, History& h) {
    h.clear();
    for (const auto& item : j) {
        h.append(item.get<SceneMessage>());
    }
}

} // namespace rhapsode
