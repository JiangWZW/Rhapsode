#include "rhapsode/weaver.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <unordered_set>

namespace rhapsode {

// ---------------------------------------------------------------------------
// Entity-group expiry detector
// ---------------------------------------------------------------------------

void Weaver::rebuild_expiry_queue(
        const std::vector<std::string>& priority_entities) {
    expiry_queue_.clear();
    auto groups = graph_.entity_groups();

    std::set<std::string> priority_keys;
    for (const auto& entity : priority_entities) {
        priority_keys.insert(str::to_lower(entity));
    }

    // Partition into priority and non-priority groups; deduplicate.
    std::vector<std::vector<std::uint64_t>> priority_groups, normal_groups;
    std::set<std::vector<std::uint64_t>> seen_groups;
    for (auto& [entity, node_ids] : groups) {
        if (node_ids.size() < 2) continue;
        auto canonical = node_ids;
        std::sort(canonical.begin(), canonical.end());
        if (!seen_groups.insert(std::move(canonical)).second) continue;

        if (priority_keys.count(str::to_lower(entity)))
            priority_groups.push_back(std::move(node_ids));
        else
            normal_groups.push_back(std::move(node_ids));
    }

    // drain_expiry_queue pops from the back, so priority goes last
    expiry_queue_ = std::move(normal_groups);
    expiry_queue_.insert(expiry_queue_.end(),
                         std::make_move_iterator(priority_groups.begin()),
                         std::make_move_iterator(priority_groups.end()));

    if (!expiry_queue_.empty())
        log() << "  [expiry] queue rebuilt: "
              << expiry_queue_.size() << " group(s), "
              << priority_groups.size() << " priority\n";
}

std::vector<ExpiryOp> Weaver::drain_expiry_queue(int turn_index) {
    expiry_stop_.store(false, std::memory_order_relaxed);
    std::vector<ExpiryOp> all;

    while (!expiry_queue_.empty()
           && !expiry_stop_.load(std::memory_order_relaxed)) {
        auto node_ids = std::move(expiry_queue_.back());
        expiry_queue_.pop_back();

        std::vector<const Node*> live_nodes;
        for (const auto node_id : node_ids) {
            const Node* node = graph_.get_node(node_id);
            if (node && node->valid_until == -1)
                live_nodes.push_back(node);
        }
        if (live_nodes.size() < 2) continue;

        auto expired = check_group(std::move(live_nodes), turn_index);
        all.insert(all.end(),
                   std::make_move_iterator(expired.begin()),
                   std::make_move_iterator(expired.end()));
    }
    return all;
}

void Weaver::stop_expiry_drain() {
    expiry_stop_.store(true, std::memory_order_relaxed);
}

bool Weaver::expiry_queue_empty() const {
    return expiry_queue_.empty();
}

std::vector<ExpiryOp> Weaver::check_group(
    std::vector<const Node*> live_nodes, int turn_index) {
    std::sort(live_nodes.begin(), live_nodes.end(),
              [](const Node* a, const Node* b) {
                  return a->created_at > b->created_at;
              });

    std::ostringstream os;
    os << "Active facts (newest first):\n";
    for (const auto* node : live_nodes)
        os << "- [" << node->id << "] (turn " << node->created_at
           << ") \"" << node->fact << "\"\n";

    os << "\nWhich of these facts are NO LONGER TRUE given the full set?\n"
          "A fact is superseded only if a newer fact makes it factually false.\n"
          "Story progression (A happened, then B happened) where both remain "
          "true is NOT supersession.\n"
          "For each superseded fact, state which newer fact supersedes it.\n"
          "Output ONLY a JSON object:\n"
          "{\"superseded\": [{\"id\": <old>, \"by\": <newer>}], \"reason\": \"...\"}\n"
          "If all are still true: {\"superseded\": [], \"reason\": \"all current\"}\n";

    if (!llm_cb_) {
        log() << "  [expiry] no LLM callback -- skipping group\n";
        return {};
    }

    std::string response;
    try {
        response = llm_cb_(sanitize_utf8(os.str()));
    } catch (const std::exception& e) {
        log() << "  [expiry] LLM call failed: " << e.what() << "\n";
        return {};
    }

    std::unordered_set<std::uint64_t> valid_ids;
    for (const auto* node : live_nodes)
        valid_ids.insert(node->id);

    const auto parsed = try_parse_json(response);
    const auto reason = parsed.value("reason", std::string{});

    std::vector<ExpiryOp> expired;
    const auto superseded =
        parsed.value("superseded", nlohmann::json::array());
    if (!superseded.is_array()) return expired;

    for (const auto& element : superseded) {
        std::uint64_t old_id = 0;
        int valid_until = turn_index;

        if (element.is_object()) {
            old_id = json_number<std::uint64_t>(element, "id", 0);
            const auto by_id =
                json_number<std::uint64_t>(element, "by", 0);
            if (by_id != 0) {
                const Node* by_node = graph_.get_node(by_id);
                if (by_node) valid_until = by_node->created_at;
            }
        } else if (element.is_number_integer()) {
            old_id = element.get<std::uint64_t>();
        }

        if (old_id == 0 || !valid_ids.count(old_id)) continue;
        if (graph_.set_valid_until(old_id, valid_until))
            expired.push_back({old_id, reason});
    }

    if (!expired.empty())
        log() << "  [expiry] " << expired.size()
              << " fact(s) superseded: " << reason << "\n";

    return expired;
}

}  // namespace rhapsode
