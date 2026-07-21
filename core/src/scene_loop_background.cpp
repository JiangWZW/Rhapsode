#include "rhapsode/scene_loop.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/world.h"

#include <future>
#include <optional>
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

std::future<SceneLoop::BackgroundResult> SceneLoop::dispatch_background(
    SceneData& scene,
    int turn,
    const DirectorOutput&) {
    Weaver* const weaver = weaver_.get();
    const auto history = scene.history.snapshot(window_size_);
    const std::string context =
        format_graph_seed(history, scene.title, kGraphSeedMaxMessageChars);
    const bool full_weave = weaver && weaver->should_weave(turn);
    const LLMCallback downsampler_callback = downsampler_cb_;

    return std::async(std::launch::async,
        [this, &scene, weaver, turn, context, full_weave, downsampler_callback]() {
            BackgroundResult result;
            if (weaver) {
                if (full_weave) {
                    log() << "  [bg] full graph weave (cloud)...\n" << std::flush;
                    result.weave = weaver->weave(turn, context);
                } else {
                    log() << "  [bg] quick graph weave (local)...\n" << std::flush;
                    result.weave = weaver->weave_local(turn, context);
                }
                if (!weaver->expiry_queue_empty())
                    result.expiry = weaver->drain_expiry_queue(turn);
            }

            world_.reflect_perceptions(turn);

            if (downsampler_callback) {
                try {
                    const int before = scene.downsampler.summarized_up_to();
                    scene.downsampler.process_turn(
                        scene.history.messages(), downsampler_callback);
                    const int after = scene.downsampler.summarized_up_to();
                    log() << "  [downsampler] summarized_up_to " << before
                          << " -> " << after << "\n";
                    const auto rendered = scene.downsampler.render();
                    if (!rendered.empty())
                        log() << "  [downsampler] story_so_far (" << rendered.size()
                              << " chars): " << rendered.substr(0, 200)
                              << (rendered.size() > 200 ? "..." : "") << "\n";
                } catch (const std::exception& error) {
                    log() << "  [downsampler] process_turn failed: "
                          << error.what() << "\n";
                }
            }
            return result;
        });
}

SceneLoop::BackgroundResult SceneLoop::finish_background(
    std::future<BackgroundResult>& future) noexcept {
    BackgroundResult completed;
    if (!future.valid()) return completed;
    try {
        completed = future.get();
    } catch (const std::exception& error) {
        log() << "  [bg] background work failed: " << error.what() << "\n";
    } catch (...) {
        log() << "  [bg] background work failed with an unknown exception\n";
    }

    if (!completed.weave.connected.empty() ||
        !completed.weave.disconnected.empty() ||
        !completed.weave.reweighted.empty()) {
        log() << "  [weave] +" << completed.weave.connected.size()
              << " -" << completed.weave.disconnected.size()
              << " ~" << completed.weave.reweighted.size() << "\n";
    }
    return completed;
}

}  // namespace rhapsode
