#include "rhapsode/turn_executor.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/world.h"

#include <utility>

namespace rhapsode {
namespace {

constexpr size_t kGraphSeedMessages = 4;
constexpr size_t kGraphSeedMaxMessageChars = 300;

std::string format_graph_seed(const std::vector<SceneMessage>& history,
                              const std::string& title,
                              std::optional<size_t> cap_per_message = std::nullopt) {
    std::string output = title;
    const size_t start = history.size() > kGraphSeedMessages
        ? history.size() - kGraphSeedMessages : 0;
    for (size_t i = start; i < history.size(); ++i) {
        const auto& message = history[i];
        if (message.role != Role::User && message.role != Role::Assistant) continue;
        output += '\n';
        output += message.role == Role::User ? "user: " : "assistant: ";
        output += cap_per_message
            ? truncate_utf8(message.content, *cap_per_message)
            : message.content;
    }
    return output;
}

}  // namespace

TurnExecutor::PostTurnResult TurnExecutor::run_post_turn(
    SceneData& scene, int turn) noexcept {
    PostTurnResult result;
    const auto history = snapshot_history(scene.history, window_size_);
    const std::string context =
        format_graph_seed(history, scene.title, kGraphSeedMaxMessageChars);
    try {
        WeaveResult weave;
        if (weaver_.active()) {
            if (weaver_.should_weave(turn)) {
                log() << "  [post-turn] full graph weave (cloud)...\n"
                      << std::flush;
                weave = weaver_.weave(turn, context);
            } else {
                log() << "  [post-turn] quick graph weave (local)...\n"
                      << std::flush;
                weave = weaver_.weave_local(turn, context);
            }
            if (!weaver_.expiry_queue_empty()) {
                const auto expiry = weaver_.drain_expiry_queue(turn);
                for (const auto& operation : expiry) {
                    if (const Node* node = world_.graph().get_node(operation.id))
                        result.expired_nodes.push_back(*node);
                }
            }
        }

        world_.reflect_perceptions(turn, reflection_llm_cb_);

        if (downsampler_cb_) {
            try {
                const int before = scene.downsampling.summarized_up_to;
                process_text_downsampling(
                    scene.downsampling, scene.history, downsampler_cb_);
                const int after = scene.downsampling.summarized_up_to;
                log() << "  [downsampler] summarized_up_to " << before
                      << " -> " << after << "\n";
                const auto rendered = render_text_downsampling(scene.downsampling);
                if (!rendered.empty())
                    log() << "  [downsampler] story_so_far (" << rendered.size()
                          << " chars): " << rendered.substr(0, 200)
                          << (rendered.size() > 200 ? "..." : "") << "\n";
            } catch (const std::exception& error) {
                log() << "  [downsampler] process_turn failed: "
                      << error.what() << "\n";
            }
        }

        if (!weave.connected.empty() || !weave.disconnected.empty() ||
            !weave.reweighted.empty()) {
            log() << "  [weave] +" << weave.connected.size()
                  << " -" << weave.disconnected.size()
                  << " ~" << weave.reweighted.size() << "\n";
        }
    } catch (const std::exception& error) {
        result = {};
        log() << "  [post-turn] work failed: " << error.what() << "\n";
    } catch (...) {
        result = {};
        log() << "  [post-turn] work failed with an unknown exception\n";
    }
    return result;
}

}  // namespace rhapsode
