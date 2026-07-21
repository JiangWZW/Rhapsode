#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

#include "rhapsode/character_memory.h"
#include "rhapsode/node.h"
#include "rhapsode/story.h"

using namespace rhapsode;
using json = nlohmann::json;

namespace {

SceneMessage stamped(Role role, const std::string& content,
                     const std::string& timestamp) {
    SceneMessage message;
    message.role = role;
    message.content = content;
    message.timestamp = timestamp;
    return message;
}

Story build_fixture() {
    SceneData scene;
    scene.scene_id = "golden";
    scene.title = "Golden Hall";
    scene.system_prompt = "Narrate the golden hall.";
    scene.turn_index = 3;

    World world;
    world.enter_character("golden", Character{"Player", "The visitor", true});
    world.enter_character("golden", Character{"Alice", "A wary guard", false});
    world.enter_character("golden", Character{"Bob", "A restless scout", false});

    CharacterMemory alice("Alice");
    const auto first = alice.seed_belief("The gate must stay shut", {"Gate"}, 0);
    const auto second = alice.seed_belief("Bob keeps eyeing the gate", {"Bob", "Gate"}, 0);
    alice.link_tension(first, second, 0);
    world.set_character_memory(std::move(alice));

    CharacterMemory bob("Bob");
    bob.seed_belief("There is a way out through the gate", {"Gate"}, 0);
    world.set_character_memory(std::move(bob));

    Node gate;
    gate.fact = "The gate is barred at dusk";
    gate.entities = {"Gate"};
    world.graph().add_node_chained(std::move(gate), 1);
    Node draft;
    draft.fact = "A draft slips under the gate";
    draft.entities = {"Gate"};
    world.graph().add_node_chained(std::move(draft), 2);

    Node seen;
    seen.fact = "Alice spots a loosened hinge";
    seen.entities = {"Gate"};
    seen.audience = {"Alice"};
    world.route_perceptions("golden", {seen}, 3);

    scene.history.append(stamped(Role::User, "I approach the gate.",
                                 "2026-01-01T00:00:00Z"));
    scene.history.append(stamped(Role::Assistant, "The hall is cold and still.",
                                 "2026-01-01T00:00:01Z"));
    scene.dialogue.append(stamped(Role::Assistant, "Alice: Halt.",
                                  "2026-01-01T00:00:02Z"));
    return Story::from_data(std::move(scene), std::move(world));
}

json canonical_state(const Story& story) {
    const SceneData* scene = story.active_scene();
    json value;
    value["scene_id"] = scene->scene_id;
    value["title"] = scene->title;
    value["turn_index"] = scene->turn_index;
    value["world_graph"] = story.world().graph().to_json();
    value["characters"] = story.world().characters();
    value["history"] = scene->history;
    value["dialogue"] = scene->dialogue;
    json memories = json::object();
    for (const auto& [name, memory] : story.world().character_memories())
        memories[name] = memory.to_json();
    value["character_memories"] = std::move(memories);
    return value;
}

json tool_reads(Story& story) {
    const std::string id = story.active_scene_id();
    return {
        {"graph_gate", json::parse(story.dispatch_tool(id, "query_graph", R"({"query":"Gate"})"))},
        {"mind_alice", json::parse(story.dispatch_tool(id, "query_mind", R"({"character":"Alice"})"))},
        {"mind_bob", json::parse(story.dispatch_tool(id, "query_mind", R"({"character":"Bob"})"))},
        {"history_gate", json::parse(story.dispatch_tool(id, "query_history", R"({"query":"gate"})"))},
    };
}

std::string temp_saves_dir() {
    const auto directory = std::filesystem::temp_directory_path() /
        "rhapsode_golden_saves";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory.string();
}

}  // namespace

TEST_CASE("substrate: save/reload round-trip preserves state", "[substrate][golden]") {
    Story original = build_fixture();
    const json before = canonical_state(original);
    const std::string saves = temp_saves_dir();
    original.save(saves);

    SceneData shell;
    shell.scene_id = "golden";
    Story reloaded = Story::from_data(std::move(shell));
    REQUIRE(reloaded.has_save(saves));
    reloaded.load_save(saves);
    REQUIRE(canonical_state(reloaded) == before);
}

TEST_CASE("substrate: narrator read-tools survive round-trip", "[substrate][golden]") {
    Story original = build_fixture();
    const json before = tool_reads(original);
    const std::string saves = temp_saves_dir();
    original.save(saves);

    SceneData shell;
    shell.scene_id = "golden";
    Story reloaded = Story::from_data(std::move(shell));
    reloaded.load_save(saves);
    REQUIRE(tool_reads(reloaded) == before);
}

TEST_CASE("story: fork shares World without coupling SceneData to it",
          "[substrate][fork]") {
    Story story = build_fixture();
    SceneData* parent = story.active_scene();
    SceneData* child = story.fork_scene("golden", "golden_b", {"Alice"});
    REQUIRE(child != nullptr);
    REQUIRE(parent != nullptr);
    REQUIRE(child->history.size() == 0);
    REQUIRE(child->dialogue.size() == 0);
    REQUIRE(child->turn_index == 0);
    REQUIRE(story.world().find_in_scene("golden_b", "Alice") != nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Alice") == nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Bob") != nullptr);

    Node node;
    node.fact = "A second gate appears";
    story.world().graph().add_node(std::move(node));
    REQUIRE(story.world().graph().size() == 3);
}

TEST_CASE("story: owns stable storyline records", "[substrate][story]") {
    Story story = build_fixture();
    SceneData* parent = story.active_scene();
    REQUIRE(story.fork_scene("golden", "golden_b", {"Alice"}) != nullptr);
    REQUIRE(story.fork_scene("golden", "golden_b", {}) == nullptr);
    REQUIRE(story.fork_scene("missing", "golden_c", {}) == nullptr);
    story.fork_scene("golden", "golden_c", {"Bob"});
    REQUIRE(story.get_scene("golden") == parent);
    REQUIRE(story.scene_count() == 3);
}

TEST_CASE("story: merge moves cast and retires source", "[substrate][story]") {
    Story story = build_fixture();
    REQUIRE(story.fork_scene("golden", "hunt", {"Bob"}, "Hunt") != nullptr);
    REQUIRE(story.merge_scene("hunt", "golden"));
    REQUIRE(story.get_scene("hunt") == nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Bob") != nullptr);
}

TEST_CASE("story: conclude clears membership and repoints active", "[substrate][story]") {
    Story story = build_fixture();
    story.fork_scene("golden", "hunt", {"Bob"}, "Hunt");
    REQUIRE(story.conclude_scene("golden", "done"));
    REQUIRE(story.active_scene_id() == "hunt");
    REQUIRE(story.world().find_in_scene("golden", "Alice") == nullptr);
}

TEST_CASE("story: staleness tracks beats", "[substrate][story]") {
    Story story = build_fixture();
    story.fork_scene("golden", "hunt", {"Bob"}, "Hunt");
    story.note_advanced("golden");
    story.note_advanced("golden");
    const json rows = json::parse(story.tool_list_scenes());
    for (const auto& row : rows) {
        if (row["scene_id"] == "golden") REQUIRE(row["staleness"] == 0);
        if (row["scene_id"] == "hunt") REQUIRE(row["staleness"] == 2);
    }
}

TEST_CASE("story: save/reload preserves scene collection", "[substrate][story]") {
    Story story = build_fixture();
    story.fork_scene("golden", "hunt", {"Bob"}, "Hunt the intruder");
    story.get_scene("hunt")->history.append(
        stamped(Role::Assistant, "Bob slips into the woods.", "2026-01-02T00:00:00Z"));
    const std::string saves = temp_saves_dir();
    story.save(saves);

    SceneData shell;
    shell.scene_id = "golden";
    Story reloaded = Story::from_data(std::move(shell));
    reloaded.load_save(saves);
    REQUIRE(reloaded.scene_count() == 2);
    REQUIRE(reloaded.get_scene("hunt")->driving_intention == "Hunt the intruder");
    REQUIRE(reloaded.get_scene("hunt")->history.size() == 1);
    REQUIRE(reloaded.world().find_in_scene("hunt", "Bob") != nullptr);
}

TEST_CASE("substrate: fixture retains deterministic shape", "[substrate][golden]") {
    Story story = build_fixture();
    REQUIRE(story.world().characters().size() == 3);
    REQUIRE(story.world().graph().size() == 2);
    int alice_perceptions = 0;
    story.world().character_memories().at("Alice").beliefs().for_each(
        [&](const Node& node) { if (node.type == "perception") ++alice_perceptions; }, false);
    int bob_perceptions = 0;
    story.world().character_memories().at("Bob").beliefs().for_each(
        [&](const Node& node) { if (node.type == "perception") ++bob_perceptions; }, false);
    REQUIRE(alice_perceptions == 1);
    REQUIRE(bob_perceptions == 0);
    REQUIRE(story.active_scene()->history.size() == 2);
    REQUIRE(story.active_scene()->dialogue.size() == 1);
}
