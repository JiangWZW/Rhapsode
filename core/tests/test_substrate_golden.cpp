// Substrate golden / regression harness (Phase 0).
//
// Purpose: freeze the *deterministic* behaviour of the state that the
// parallel-scenes substrate refactor will move between `Scene` and the new
// `World` -- the world graph, per-character minds, roster, and their
// serialization + narrator read-tools. Nothing here calls an LLM, so every
// assertion is reproducible.
//
// The oracle is a save -> reload round-trip: after Step 2 (on_stage -> scene_id)
// and Step 3 (extract World, split the save schema), this test must stay green.
// If a refactor is truly behaviour-preserving, the reloaded state and the
// narrator tool-query outputs are byte-for-byte identical to the originals.

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/node.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop_support.h"
#include "rhapsode/story.h"

using namespace rhapsode;
using json = nlohmann::json;

namespace {

// A message with an explicit timestamp -- History::append only stamps when the
// field is empty, so setting it keeps the fixture deterministic.
SceneMessage stamped(Role role, const std::string& content,
                     const std::string& ts) {
    SceneMessage m;
    m.role = role;
    m.content = content;
    m.timestamp = ts;
    return m;
}

// Build a fixed, LLM-free scene: two NPCs on stage, authored beliefs with a
// live tension, a small chained world graph, one routed perception, and prose
// in both the narrator and dialogue threads.
Scene build_fixture() {
    Scene scene;
    scene.scene_id = "golden";
    scene.title = "Golden Hall";
    scene.system_prompt = "Narrate the golden hall.";
    scene.turn_index = 3;

    Character player{"Player", "The visitor", true};
    Character alice{"Alice", "A wary guard", false};
    alice.created_at = -1;
    Character bob{"Bob", "A restless scout", false};
    bob.created_at = -1;

    scene.world().characters.push_back(player);
    scene.enter_character(std::move(alice));  // joins "golden"
    scene.enter_character(std::move(bob));    // joins "golden"

    // Authored minds: two beliefs each, with a cross-linked tension for Alice.
    CharacterMemory alice_mem("Alice");
    auto a0 = alice_mem.seed_belief("The gate must stay shut", {"Gate"}, 0);
    auto a1 = alice_mem.seed_belief("Bob keeps eyeing the gate", {"Bob", "Gate"}, 0);
    alice_mem.link_tension(a0, a1, 0);
    scene.world().character_memories.emplace("Alice", std::move(alice_mem));

    CharacterMemory bob_mem("Bob");
    bob_mem.seed_belief("There is a way out through the gate", {"Gate"}, 0);
    scene.world().character_memories.emplace("Bob", std::move(bob_mem));

    // World graph: a short chained entity timeline for the gate.
    Node n0;
    n0.fact = "The gate is barred at dusk";
    n0.entities = {"Gate"};
    scene.world().world_graph.add_node_chained(std::move(n0), 1);
    Node n1;
    n1.fact = "A draft slips under the gate";
    n1.entities = {"Gate"};
    scene.world().world_graph.add_node_chained(std::move(n1), 2);

    // Perception routing (deterministic): a private beat only Alice sees.
    Node seen;
    seen.fact = "Alice spots a loosened hinge";
    seen.entities = {"Gate"};
    seen.audience = {"Alice"};
    route_perception(scene, {seen}, 3);

    scene.history.append(stamped(Role::User, "I approach the gate.",
                                 "2026-01-01T00:00:00Z"));
    scene.history.append(stamped(Role::Assistant, "The hall is cold and still.",
                                 "2026-01-01T00:00:01Z"));
    scene.dialogue.append(stamped(Role::Assistant, "Alice: Halt.",
                                  "2026-01-01T00:00:02Z"));

    return scene;
}

// Canonical, order-insensitive dump of the deterministic state we care about.
// Uses nlohmann's value equality (objects compared by key, not insertion order).
json canonical_state(const Scene& s) {
    json j;
    j["scene_id"] = s.scene_id;
    j["title"] = s.title;
    j["turn_index"] = s.turn_index;
    j["world_graph"] = s.world().world_graph.to_json();
    j["characters"] = s.world().characters;
    j["history"] = s.history;
    j["dialogue"] = s.dialogue;

    json minds = json::object();
    for (const auto& [name, mem] : s.world().character_memories)
        minds[name] = mem.to_json();
    j["character_memories"] = std::move(minds);
    return j;
}

// Narrator read-tools, parsed so comparison is structural (order-insensitive).
json tool_reads(const Scene& s) {
    return json{
        {"graph_gate", json::parse(s.tool_query_graph("Gate"))},
        {"mind_alice", json::parse(s.tool_query_mind("Alice"))},
        {"mind_bob", json::parse(s.tool_query_mind("Bob"))},
        {"history_gate", json::parse(s.tool_query_history("gate"))},
    };
}

std::string temp_saves_dir() {
    auto dir = std::filesystem::temp_directory_path() /
               "rhapsode_golden_saves";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir.string();
}

}  // namespace

TEST_CASE("substrate: save/reload round-trip preserves state", "[substrate][golden]") {
    Scene original = build_fixture();
    const json before = canonical_state(original);

    const std::string saves = temp_saves_dir();
    original.save(saves);

    // Reload the way the app does: `load_json` restores scenario-static fields
    // (scene_id, title, system_prompt), then `load_save` overlays the mutable
    // game state. The save deliberately omits the static fields, so the shell
    // must carry them -- this is part of the contract Step 3 must preserve.
    Scene reloaded;
    reloaded.scene_id = original.scene_id;
    reloaded.title = original.title;
    reloaded.system_prompt = original.system_prompt;
    REQUIRE(reloaded.has_save(saves));
    reloaded.load_save(saves);

    const json after = canonical_state(reloaded);
    REQUIRE(after == before);
}

TEST_CASE("substrate: narrator read-tools survive round-trip", "[substrate][golden]") {
    Scene original = build_fixture();
    const json before = tool_reads(original);

    const std::string saves = temp_saves_dir();
    original.save(saves);

    Scene reloaded;
    reloaded.scene_id = "golden";
    reloaded.load_save(saves);

    const json after = tool_reads(reloaded);
    REQUIRE(after == before);
}

TEST_CASE("substrate: fork shares the World but not the prose", "[substrate][fork]") {
    Scene parent = build_fixture();
    Scene child = parent.fork("golden_b", {"Alice"});

    // Same durable substrate: a mutation through the parent is visible in the child.
    REQUIRE(&child.world() == &parent.world());
    Node n;
    n.fact = "A second gate appears";
    n.entities = {"Gate2"};
    parent.world().world_graph.add_node_chained(std::move(n), 4);
    REQUIRE(child.world().world_graph.size() == parent.world().world_graph.size());

    // Child has its own ephemeral state.
    REQUIRE(child.scene_id == "golden_b");
    REQUIRE(child.turn_index == 0);
    REQUIRE(child.history.size() == 0);
    REQUIRE(child.dialogue.size() == 0);

    // Membership: a fork MOVES its cast. Alice leaves the parent for golden_b;
    // Bob (not in the fork) stays behind in the parent.
    const Character* alice = parent.world().find_character("Alice");
    REQUIRE(alice != nullptr);
    REQUIRE_FALSE(alice->in_scene("golden"));
    REQUIRE(alice->in_scene("golden_b"));
    REQUIRE(child.find_on_stage("Alice") != nullptr);
    REQUIRE(child.find_on_stage("Bob") == nullptr);
    REQUIRE(parent.find_on_stage("Bob") != nullptr);
}

TEST_CASE("story: hub owns storylines over one shared World", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());

    REQUIRE(story.scene_count() == 1);
    REQUIRE(story.active_scene_id() == "golden");
    REQUIRE(&story.active_scene()->world() == &story.world());

    // Fork a second storyline through the hub; it shares the same World.
    Scene* child = story.fork_scene("golden", "golden_b", {"Alice"});
    REQUIRE(child != nullptr);
    REQUIRE(story.scene_count() == 2);
    REQUIRE(&child->world() == &story.world());

    // Duplicate id and unknown parent are rejected.
    REQUIRE(story.fork_scene("golden", "golden_b", {}) == nullptr);
    REQUIRE(story.fork_scene("nope", "golden_c", {}) == nullptr);
    REQUIRE(story.scene_count() == 2);

    // A Scene* handed out stays valid across a later fork (pointer stability).
    Scene* parent = story.get_scene("golden");
    story.fork_scene("golden", "golden_c", {"Bob"});
    REQUIRE(story.get_scene("golden") == parent);

    // list_scenes is a structured digest for the scheduler.
    json rows = json::parse(story.tool_list_scenes());
    REQUIRE(rows.is_array());
    REQUIRE(rows.size() == 3);
    bool saw_active = false;
    for (const auto& row : rows) {
        REQUIRE(row.contains("scene_id"));
        REQUIRE(row.contains("cast"));
        REQUIRE(row.contains("turn_index"));
        if (row["scene_id"] == "golden") {
            REQUIRE(row["active"] == true);
            saw_active = true;
        }
        if (row["scene_id"] == "golden_b") {
            // Alice is the only cast the fork moved onto golden_b.
            REQUIRE(row["cast"].size() == 1);
            REQUIRE(row["cast"][0] == "Alice");
        }
    }
    REQUIRE(saw_active);

    // Conclude a storyline: its cast leaves that scene; other storylines keep
    // theirs. (Alice was moved onto golden_b by the fork, so she ends up in no
    // scene; Bob, moved onto golden_c, is untouched.)
    REQUIRE(story.conclude_scene("golden_b", "driving intention expired"));
    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.get_scene("golden_b") == nullptr);
    const Character* alice = story.world().find_character("Alice");
    REQUIRE_FALSE(alice->in_scene("golden_b"));
    REQUIRE(story.world().find_character("Bob")->in_scene("golden_c"));

    // Concluding the active scene repoints active to a survivor.
    REQUIRE(story.conclude_scene("golden", "done"));
    REQUIRE(story.active_scene_id() == "golden_c");
}

TEST_CASE("story: staged lifecycle ops apply after a beat", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());

    // A narrator on the root scene stages a fork; nothing changes until applied.
    std::string ack = story.world().stage_fork("golden", "Hunt the intruder", {"Bob"});
    REQUIRE(json::parse(ack)["ok"] == true);
    REQUIRE(story.scene_count() == 1);

    REQUIRE(story.apply_pending_ops() == 1);
    REQUIRE(story.scene_count() == 2);

    // The forked scene carries the drive and shows up with charge in list_scenes.
    json rows = json::parse(story.tool_list_scenes());
    const json* forked = nullptr;
    for (const auto& r : rows)
        if (r["scene_id"] != "golden") forked = &r;
    REQUIRE(forked != nullptr);
    REQUIRE((*forked)["driving_intention"] == "Hunt the intruder");
    REQUIRE((*forked)["charge"].get<float>() > 0.0f);
    REQUIRE((*forked)["player_present"] == false);

    // Cleared ops (as on a retry) never apply.
    story.world().stage_conclude("golden", "test");
    story.world().clear_pending_ops();
    REQUIRE(story.apply_pending_ops() == 0);
    REQUIRE(story.scene_count() == 2);
}

TEST_CASE("story: staged exit drops a character into no storyline", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());
    REQUIRE(story.world().find_character("Bob")->in_scene("golden"));

    // A plain exit (not a fork): Bob leaves the scene, no new storyline is made.
    std::string ack = story.world().stage_exit("golden", {"Bob"});
    REQUIRE(json::parse(ack)["ok"] == true);
    REQUIRE(story.world().find_character("Bob")->in_scene("golden"));  // staged, not applied

    REQUIRE(story.apply_pending_ops() == 1);
    REQUIRE(story.scene_count() == 1);  // exit never creates or retires a scene
    REQUIRE_FALSE(story.world().find_character("Bob")->in_scene("golden"));
}

TEST_CASE("story: merge moves cast and retires the source", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());
    Scene* child = story.fork_scene("golden", "hunt", {"Bob"}, "Hunt");
    REQUIRE(child != nullptr);

    // Merge the child back into the root: Bob joins root, child retires.
    REQUIRE(story.world().find_character("Bob")->in_scene("hunt"));
    REQUIRE(story.merge_scene("hunt", "golden"));
    REQUIRE(story.get_scene("hunt") == nullptr);
    const Character* bob = story.world().find_character("Bob");
    REQUIRE(bob->in_scene("golden"));
    REQUIRE_FALSE(bob->in_scene("hunt"));
}

TEST_CASE("story: staleness tracks beats since last advanced", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());
    story.fork_scene("golden", "hunt", {"Bob"}, "Hunt");

    story.note_advanced("golden");   // beat 0 -> golden fresh, clock now 1
    story.note_advanced("golden");   // beat 1 -> golden fresh, clock now 2

    json rows = json::parse(story.tool_list_scenes());
    for (const auto& r : rows) {
        if (r["scene_id"] == "golden") REQUIRE(r["staleness"].get<int>() == 0);
        if (r["scene_id"] == "hunt")   REQUIRE(r["staleness"].get<int>() == 2);
    }
}

TEST_CASE("story: save/reload round-trip preserves the scene set", "[substrate][story]") {
    Story story = Story::from_scene(build_fixture());
    story.fork_scene("golden", "hunt", {"Bob"}, "Hunt the intruder");
    story.get_scene("hunt")->history.append(
        stamped(Role::Assistant, "Bob slips into the woods.", "2026-01-02T00:00:00Z"));
    story.note_advanced("golden");
    story.set_active_scene("golden");

    const std::string saves = temp_saves_dir();
    story.save(saves);

    // Rebuild the way the app does: load the root scene, wrap in a Story, resume.
    Scene root;
    root.scene_id = "golden";
    Story reloaded = Story::from_scene(std::move(root));
    REQUIRE(reloaded.has_save(saves));
    reloaded.load_save(saves);

    REQUIRE(reloaded.scene_count() == 2);
    REQUIRE(reloaded.active_scene_id() == "golden");
    REQUIRE(reloaded.get_scene("hunt") != nullptr);
    REQUIRE(reloaded.get_scene("hunt")->driving_intention == "Hunt the intruder");
    REQUIRE(reloaded.get_scene("hunt")->history.size() == 1);
    // Membership survived the round-trip: the fork moved Bob onto "hunt".
    const Character* bob = reloaded.world().find_character("Bob");
    REQUIRE(bob != nullptr);
    REQUIRE(bob->in_scene("hunt"));
    REQUIRE_FALSE(bob->in_scene("golden"));
}

TEST_CASE("substrate: fixture holds the expected deterministic shape", "[substrate][golden]") {
    Scene s = build_fixture();

    // Roster: player + two on-stage NPCs.
    REQUIRE(s.world().characters.size() == 3);
    REQUIRE(s.find_on_stage("Alice") != nullptr);
    REQUIRE(s.find_on_stage("Bob") != nullptr);

    // World graph: two authored gate nodes, chained.
    REQUIRE(s.world().world_graph.size() == 2);

    // Perception routing respected the private audience: Alice saw it, Bob did not.
    int alice_perceptions = 0;
    s.world().character_memories.at("Alice").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++alice_perceptions;
    }, false);
    int bob_perceptions = 0;
    s.world().character_memories.at("Bob").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++bob_perceptions;
    }, false);
    REQUIRE(alice_perceptions == 1);
    REQUIRE(bob_perceptions == 0);

    // Prose threads are kept separate.
    REQUIRE(s.history.size() == 2);
    REQUIRE(s.dialogue.size() == 1);
}
