#include "rhapsode/scene_loop.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene.h"

#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <utility>

namespace rhapsode {
namespace {

constexpr size_t kGraphSeedMessages = 4;
constexpr size_t kGraphSeedMaxMessageChars = 300;

std::string format_graph_seed(const std::vector<SceneMessage>& history,
                              const std::string& title,
                              std::optional<size_t> cap_per_msg = std::nullopt) {
    std::string out = title;
    const size_t start = history.size() > kGraphSeedMessages
                             ? history.size() - kGraphSeedMessages
                             : 0;
    for (size_t i = start; i < history.size(); ++i) {
        const auto& msg = history[i];
        if (msg.role != Role::User && msg.role != Role::Assistant)
            continue;
        out += '\n';
        out += (msg.role == Role::User ? "user: " : "assistant: ");
        out += cap_per_msg ? truncate_utf8(msg.content, *cap_per_msg) : msg.content;
    }
    return out;
}

}  // namespace

void SceneLoop::join_background() {
    if (bg_stop_) {
        try { bg_stop_(); } catch (...) {}
        bg_stop_ = {};
    }

    BackgroundResult completed;
    if (bg_future_.valid()) {
        try { completed = bg_future_.get(); }
        catch (const std::exception& e) {
            log() << "  [bg] background work failed: " << e.what() << "\n";
        }
        catch (...) {
            log() << "  [bg] background work failed with an unknown exception\n";
        }
    }

    last_weave_result_ = std::move(completed.weave);
    completed_expiry_ops_ = std::move(completed.expiry);

    if (!last_weave_result_.connected.empty()
        || !last_weave_result_.disconnected.empty()
        || !last_weave_result_.reweighted.empty()) {
        log() << "  [weave] +"
              << last_weave_result_.connected.size()
              << " -" << last_weave_result_.disconnected.size()
              << " ~" << last_weave_result_.reweighted.size() << "\n";
    }

    if (background_save_pending_) {
        background_save_pending_ = false;
        if (!saves_dir_.empty() && scene_) scene_->save(saves_dir_);
    }
}

std::vector<ExpiryOp> SceneLoop::take_completed_expiry_ops() {
    return std::exchange(completed_expiry_ops_, {});
}

void SceneLoop::dispatch_background() {
    Scene* const scene = scene_;
    Weaver* const weaver = weaver_;
    const auto history = scene->history.snapshot(window_size_);
    const std::string ctx =
        format_graph_seed(history, scene->title, kGraphSeedMaxMessageChars);
    int turn = scene->turn_index;

    bool has_weaver = weaver != nullptr;
    bool full_weave = has_weaver && weaver->should_weave(turn);

    bg_stop_ = has_weaver
        ? std::function<void()>([weaver]() { weaver->stop_expiry_drain(); })
        : std::function<void()>{};

    bg_future_ = std::async(std::launch::async,
        [scene, weaver, turn, ctx = std::move(ctx), has_weaver, full_weave]()
            -> BackgroundResult
    {
        BackgroundResult result;
        if (has_weaver) {
            if (full_weave) {
                log() << "  [bg] full graph weave (cloud)...\n" << std::flush;
                result.weave = weaver->weave(turn, ctx);
            } else {
                log() << "  [bg] quick graph weave (local)...\n" << std::flush;
                result.weave = weaver->weave_local(turn, ctx);
            }

            if (!weaver->expiry_queue_empty())
                result.expiry = weaver->drain_expiry_queue(turn);
        }

        // Consolidate this turn's routed perceptions into beliefs (no-op for
        // minds that perceived nothing).
        scene->world().reflect_perceptions(turn);

        // Downsample history off the foreground (thinking-on, multi-call). Safe
        // here: the next turn's join_background() completes this before the
        // prompt callback reads downsampler.render(), and history is not mutated
        // while the main thread waits for player input.
        if (scene->downsampler.has_llm_callback()) {
            try {
                int before = scene->downsampler.summarized_up_to();
                scene->downsampler.process_turn(scene->history.messages());
                int after = scene->downsampler.summarized_up_to();
                log() << "  [downsampler] summarized_up_to " << before
                      << " -> " << after << "\n";
                auto rendered = scene->downsampler.render();
                if (!rendered.empty())
                    log() << "  [downsampler] story_so_far (" << rendered.size()
                          << " chars): " << rendered.substr(0, 200)
                          << (rendered.size() > 200 ? "..." : "") << "\n";
            } catch (const std::exception& e) {
                log() << "  [downsampler] process_turn failed: " << e.what() << "\n";
            }
        }
        return result;
    });
}

}  // namespace rhapsode
