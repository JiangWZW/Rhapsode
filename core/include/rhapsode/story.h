#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "rhapsode/llm_callback.h"
#include "rhapsode/scene.h"
#include "rhapsode/world.h"

namespace rhapsode {

class SceneLoop;
struct SceneTurnResult;

// Picks the next off-stage scene to advance. The engine supplies the scheduler
// instructions and the user prompt; the Python side runs the tool-use loop and
// returns the chosen scene_id (empty string = advance nothing).
using SchedulerCallback = std::function<std::string(const std::string& instructions,
                                                    const std::string& user)>;

// Renders the lifecycle verdict for one beat: given the engine's instructions and
// a description of what just happened, returns a JSON verdict (fork/merge/
// conclude/exit or nothing). This is the sole authority on cross-scene membership
// -- a focused decision, not an optional tool the narrator may skip.
using LifecycleCallback = std::function<std::string(const std::string& instructions,
                                                    const std::string& user)>;

// The runtime hub for one playthrough. A Story runs over exactly one durable
// World and owns the set of live Scenes (storylines) drawn on that World.
//
// Ownership is a tree, not a cycle: the Story co-owns the World (shared_ptr) and
// owns every Scene (unique_ptr); each Scene co-owns the same World. Dropping the
// Story drops its Scenes, then the World. Because Scenes are held by pointer,
// forking or concluding a storyline never moves the others, so a Scene* handed
// out to the loop stays valid across lifecycle changes.
class Story {
public:
    Story() : world_(std::make_shared<World>()) {}

    // Move-only: the Story owns its Scenes (unique_ptr) and is never copied.
    // Explicit deletes keep MSVC/pybind11's copy-constructibility probe from
    // instantiating the vector-of-unique_ptr copy constructor.
    Story(const Story&) = delete;
    Story& operator=(const Story&) = delete;
    Story(Story&&) = default;
    Story& operator=(Story&&) = default;

    // Adopt a loaded scene as the root storyline; its World becomes the Story's
    // World. Every later scene is a fork over that same World.
    static Story from_scene(Scene root);

    // -- Shared substrate --
    World& world() { return *world_; }
    const World& world() const { return *world_; }
    std::shared_ptr<World> world_ptr() const { return world_; }

    // -- Scene collection --
    Scene* get_scene(const std::string& id);
    const Scene* get_scene(const std::string& id) const;
    std::vector<std::string> scene_ids() const;
    size_t scene_count() const { return scenes_.size(); }

    const std::string& active_scene_id() const { return active_scene_id_; }
    void set_active_scene(const std::string& id) { active_scene_id_ = id; }
    Scene* active_scene() { return get_scene(active_scene_id_); }

    // -- Lifecycle --
    /// Fork `parent_id` into a new storyline `new_id` over the shared World,
    /// moving the named cast onto it and setting its driving intention. Returns
    /// the child, or nullptr if the parent is unknown or `new_id` already exists.
    Scene* fork_scene(const std::string& parent_id,
                      const std::string& new_id,
                      const std::vector<std::string>& cast,
                      const std::string& driving_intention = "");

    /// Retire storyline `id`: its cast leaves that scene's membership and the
    /// projection is dropped. Returns false if the scene is unknown. The active
    /// scene is repointed to the first remaining storyline, if any.
    bool conclude_scene(const std::string& id, const std::string& reason);

    /// Merge `from_id` into `into_id`: the source's cast joins the target's
    /// stage, then the source projection is retired. Returns false if either
    /// scene is unknown.
    bool merge_scene(const std::string& from_id, const std::string& into_id);

    /// Drain the World's staged lifecycle ops and apply them to the scene set.
    /// Called by the orchestrator after a beat is accepted. Returns the number
    /// of ops applied.
    int apply_pending_ops();

    // -- Scheduler bookkeeping --
    /// Record that `scene_id` just advanced; stamps its `last_advanced` with the
    /// current beat clock and ticks the clock. Feeds the `staleness` column.
    void note_advanced(const std::string& scene_id);
    int beat_clock() const { return beat_clock_; }

    // -- Scheduler read tool --
    /// JSON array, one object per live scene, each with: scene_id, title,
    /// active, player_present, turn_index, staleness, cast[], driving_intention,
    /// charge, last_narration. Structured rows for the scheduler to read and
    /// drill into, rebuilt fresh on every call.
    std::string tool_list_scenes() const;

    // -- Tool dispatch --
    /// Execute one narrator/scheduler read-or-decision tool against this Story
    /// and return its JSON result. `scene_id` is the beat's own scene (used by
    /// query_history and the decision tools); `args_json` is the tool call's
    /// arguments object. The Python LLM adapters forward every tool call here so
    /// the tool bodies live in one place, in the engine.
    std::string dispatch_tool(const std::string& scene_id,
                              const std::string& name,
                              const std::string& args_json);

    // -- Runtime (injected, not owned) --
    /// Bind the SceneLoop the engine drives. The loop keeps its own Director,
    /// Weaver, and LLM callbacks; the Story only sequences beats over it. The
    /// caller retains ownership; the loop must outlive the Story's use of it.
    void bind_runtime(SceneLoop& loop) { loop_ = &loop; }
    void set_scheduler_callback(SchedulerCallback cb) { scheduler_cb_ = std::move(cb); }
    /// The lifecycle verdict adapter, run once after every beat to decide any
    /// fork/merge/conclude/exit for the beat's scene.
    void set_lifecycle_callback(LifecycleCallback cb) { lifecycle_cb_ = std::move(cb); }
    /// LLM callback applied to each scene's downsampler as beats are pointed at
    /// it (forked scenes start with a fresh downsampler that needs wiring).
    void set_downsampler_callback(LLMCallback cb) { downsampler_cb_ = std::move(cb); }
    void set_saves_dir(const std::string& dir) { saves_dir_ = dir; }

    // -- The turn --
    /// Advance the player's active scene with `player_input`, apply any staged
    /// lifecycle ops, sync memory, then advance at most one off-stage scene the
    /// scheduler selects. Persists the whole story and returns the player scene's
    /// turn outputs (narrator prose + NPC lines) for the caller to stream.
    std::vector<SceneMessage> advance_scene(const std::string& player_input);

    // -- Persistence (world.json + per-scene blobs + story.json manifest) --
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

private:
    Scene* adopt(Scene s);
    std::string derive_intention(const Scene& s, float* charge_out) const;

    // Ask the scheduler which off-stage scene to advance; "" for none.
    std::string pick_off_stage_scene();
    // Run the lifecycle verdict for the beat just authored on `scene_id` and stage
    // any fork/merge/conclude/exit it returns. `player_input` is the player's line
    // for a player beat, empty for an off-stage beat.
    void decide_lifecycle(const std::string& scene_id, const std::string& player_input);
    // The director cue that drives an off-stage (player-less) beat on `scene_id`.
    std::string autonomous_cue(const std::string& scene_id) const;
    // Push a completed scene turn's new/expired facts into the shared
    // MemorySystem (ChromaDB).
    void sync_beat(const SceneTurnResult& result);

    std::shared_ptr<World> world_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    std::string active_scene_id_;
    int beat_clock_ = 0;

    // Injected runtime + Python LLM adapters (not owned).
    SceneLoop* loop_ = nullptr;
    SchedulerCallback scheduler_cb_;
    LifecycleCallback lifecycle_cb_;
    LLMCallback downsampler_cb_;
    std::string saves_dir_;
};

} // namespace rhapsode
