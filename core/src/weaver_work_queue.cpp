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

namespace {

constexpr std::size_t kMaxExpiryBatchGroups = 8;
constexpr std::size_t kMaxExpiryBatchNodes = 80;
constexpr std::size_t kMaxExpiryBatchPromptChars = 16000;
constexpr std::size_t kExpiryPromptOverhead = 1024;
constexpr std::size_t kExpiryPromptCharsPerNode = 64;

}  // namespace

// ---------------------------------------------------------------------------
// Entity-group expiry detector
// ---------------------------------------------------------------------------

void Weaver::rebuild_expiry_queue(
        const WorldGraph& graph,
        const std::vector<std::string>& priority_entities) {
    expiry_queue_.clear();
    auto groups = graph.entity_groups();

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

std::vector<ExpiryOp> Weaver::drain_expiry_queue(
    WorldGraph& graph, int /*turn_index*/) {
    expiry_stop_.store(false, std::memory_order_relaxed);
    std::vector<ExpiryOp> all;

    while (!expiry_queue_.empty()
           && !expiry_stop_.load(std::memory_order_relaxed)) {
        std::vector<std::vector<std::uint64_t>> batch;
        std::unordered_set<std::uint64_t> batch_ids;
        std::size_t node_count = 0;
        std::size_t prompt_chars = kExpiryPromptOverhead;

        while (!expiry_queue_.empty()) {
            std::vector<std::uint64_t> live_ids;
            std::size_t group_chars = 0;
            for (const auto node_id : expiry_queue_.back()) {
                const Node* node = graph.get_node(node_id);
                if (!node || node->state != NodeState::Active ||
                    node->valid_until != -1) {
                    continue;
                }
                live_ids.push_back(node_id);
                group_chars += node->fact.size() + kExpiryPromptCharsPerNode;
            }

            if (live_ids.size() < 2) {
                expiry_queue_.pop_back();
                continue;
            }

            const bool overlaps = std::any_of(
                live_ids.begin(), live_ids.end(),
                [&](const auto id) { return batch_ids.count(id) != 0; });
            const bool exceeds_limit =
                batch.size() >= kMaxExpiryBatchGroups ||
                node_count + live_ids.size() > kMaxExpiryBatchNodes ||
                prompt_chars + group_chars > kMaxExpiryBatchPromptChars;
            if (!batch.empty() && (overlaps || exceeds_limit)) break;

            expiry_queue_.pop_back();
            node_count += live_ids.size();
            prompt_chars += group_chars;
            batch_ids.insert(live_ids.begin(), live_ids.end());
            batch.push_back(std::move(live_ids));
        }

        if (batch.empty()) continue;
        auto expired = check_batch(graph, batch);
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

std::vector<ExpiryOp> Weaver::check_batch(
    WorldGraph& graph,
    const std::vector<std::vector<std::uint64_t>>& groups) {
    std::ostringstream os;
    os << "Expiry check for active fact groups.\n";
    for (std::size_t i = 0; i < groups.size(); ++i) {
        std::vector<const Node*> live_nodes;
        for (const auto id : groups[i]) {
            const Node* node = graph.get_node(id);
            if (node && node->state == NodeState::Active &&
                node->valid_until == -1) {
                live_nodes.push_back(node);
            }
        }
        std::sort(live_nodes.begin(), live_nodes.end(),
                  [](const Node* a, const Node* b) {
                      return a->created_at > b->created_at;
                  });

        os << "\nGroup g" << i << " (newest first):\n";
        for (const auto* node : live_nodes)
            os << "- [" << node->id << "] (turn " << node->created_at
               << ") \"" << node->fact << "\"\n";
    }

    os << "\nWhich facts are NO LONGER TRUE within their group?\n"
          "A fact is superseded only if a newer fact makes it factually false.\n"
          "Story progression (A happened, then B happened) where both remain "
          "true is NOT supersession.\n"
          "For each superseded fact, state which newer fact supersedes it.\n"
          "Output ONLY a JSON object:\n"
          "{\"results\":[{\"group_id\":\"g0\",\"superseded\":"
          "[{\"id\":<old>,\"by\":<newer>}],\"reason\":\"...\"}]}\n"
          "Include one result per group; use an empty superseded array when all "
          "facts in a group remain current.\n";

    if (!llm_cb_) {
        log() << "  [expiry] no LLM callback -- skipping batch\n";
        return {};
    }

    std::string response;
    try {
        response = llm_cb_(sanitize_utf8(os.str()));
    } catch (const std::exception& e) {
        log() << "  [expiry] LLM call failed: " << e.what() << "\n";
        return {};
    }

    std::vector<ExpiryOp> expired;
    const auto parsed = try_parse_json(response);
    if (!parsed.is_object()) return expired;
    const auto results_it = parsed.find("results");
    if (results_it == parsed.end() || !results_it->is_array()) {
        return expired;
    }

    std::vector<const nlohmann::json*> group_results(groups.size(), nullptr);
    for (const auto& result : *results_it) {
        if (!result.is_object()) continue;
        const auto group_id_it = result.find("group_id");
        if (group_id_it == result.end() || !group_id_it->is_string()) continue;
        const auto& group_id = group_id_it->get_ref<const std::string&>();
        for (std::size_t i = 0; i < groups.size(); ++i) {
            if (!group_results[i] && group_id == "g" + std::to_string(i)) {
                group_results[i] = &result;
                break;
            }
        }
    }

    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (!group_results[i]) continue;
        const auto superseded_it = group_results[i]->find("superseded");
        if (superseded_it == group_results[i]->end() ||
            !superseded_it->is_array()) {
            continue;
        }

        const std::unordered_set<std::uint64_t> valid_ids(
            groups[i].begin(), groups[i].end());
        const auto reason_it = group_results[i]->find("reason");
        const std::string reason =
            reason_it != group_results[i]->end() && reason_it->is_string()
                ? reason_it->get<std::string>()
                : std::string{};
        for (const auto& element : *superseded_it) {
            if (!element.is_object()) continue;
            std::uint64_t old_id = 0;
            std::uint64_t by_id = 0;
            try {
                old_id = json_number<std::uint64_t>(element, "id", 0);
                by_id = json_number<std::uint64_t>(element, "by", 0);
            } catch (...) {
                continue;
            }
            if (!valid_ids.count(old_id) || !valid_ids.count(by_id)) continue;

            const Node* old_node = graph.get_node(old_id);
            const Node* by_node = graph.get_node(by_id);
            if (!old_node || !by_node ||
                old_node->state != NodeState::Active ||
                old_node->valid_until != -1 ||
                by_node->created_at <= old_node->created_at) {
                continue;
            }
            if (graph.set_valid_until(old_id, by_node->created_at))
                expired.push_back({old_id, reason});
        }
    }

    if (!expired.empty())
        log() << "  [expiry] " << expired.size()
              << " fact(s) superseded\n";

    return expired;
}

}  // namespace rhapsode
