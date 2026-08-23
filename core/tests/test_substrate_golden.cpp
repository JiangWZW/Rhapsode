#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "rhapsode/character_memory.h"
#include "rhapsode/node.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story.h"
#include "rhapsode/text_downsampling.h"

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

void configure_fork_synthesis(Story& story) {
    story.set_narrator_llm_callback(
        [](const std::string&, const std::string& instructions,
           const std::string&, const ReadToolCallback&) {
            if (instructions.find("fork_story_so_far") != std::string::npos) {
                return std::string{
                    R"({"fork_story_so_far":"The departing cast leaves the Golden Hall with its purpose unresolved."})"};
            }
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                return std::string{
                    "<<<RHAPSODE_JSON>>>\n{\"transitions\":[],\"new_nodes\":[]}"};
            }
            return std::string{"unexpected narrator call"};
        });
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
    alice.seed_belief("Alice spots a loosened hinge", {"Gate"}, 3,
                      CharacterMemory::kAuthoredSeedWeight, "perception");
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

    append_history_message(scene.history, stamped(
        Role::User, "I approach the gate.", "2026-01-01T00:00:00Z"));
    append_history_message(scene.history, stamped(
        Role::Assistant, "The hall is cold and still.",
        "2026-01-01T00:00:01Z"));
    append_history_message(scene.dialogue, stamped(
        Role::Assistant, "Alice: Halt.", "2026-01-01T00:00:02Z"));
    Story story = Story::from_data(std::move(scene), std::move(world));
    configure_fork_synthesis(story);
    return story;
}

json canonical_state(const Story& story) {
    const SceneData* scene = story.active_scene();
    json value;
    value["scene_id"] = scene->scene_id;
    value["title"] = scene->title;
    value["turn_index"] = scene->turn_index;
    value["state_version"] = story.state_version();
    value["world_graph"] = story.observations().to_json();
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
    story.note_advanced("golden");
    bool narrator_called = false;
    story.set_narrator_llm_callback(
        [&](const std::string& scene_id, const std::string& instructions,
            const std::string& turn_state, const ReadToolCallback& read_tool) {
            narrator_called = true;
            REQUIRE(scene_id == "golden");
            REQUIRE(instructions.find("fork_story_so_far") != std::string::npos);
            const json context = json::parse(turn_state);
            REQUIRE(context["parent"]["scene_id"] == "golden");
            REQUIRE(context["fork"]["cast"] == json::array({"Alice"}));
            REQUIRE(context["fork"]["driving_intention"] == "Guard the road");
            REQUIRE(context["parent"]["recent_timeline"].size() == 3);
            REQUIRE(json::parse(read_tool(
                "query_mind", R"({"character":"Alice"})"))["character"] ==
                    "Alice");
            return std::string{
                R"({"fork_story_so_far":"Alice leaves the cold hall to guard the road while the barred gate remains unresolved."})"};
        });
    SceneData* parent = story.active_scene();
    SceneData* child = story.fork_scene(
        "golden", "golden_b", {"Alice"}, "Guard the road");
    REQUIRE(child != nullptr);
    REQUIRE(narrator_called);
    REQUIRE(parent != nullptr);
    REQUIRE(child->history.size() == 0);
    REQUIRE(child->dialogue.size() == 0);
    REQUIRE(child->turn_index == 0);
    REQUIRE(render_text_downsampling(child->downsampling) ==
            "Alice leaves the cold hall to guard the road while the barred gate remains unresolved.");
    REQUIRE(child->intention_owner == "Alice");
    REQUIRE(child->intention_node_id != 0);
    const Node* intention = story.world().character_memories().at("Alice")
        .beliefs().get_node(child->intention_node_id);
    REQUIRE(intention != nullptr);
    REQUIRE(intention->type == "intention");
    REQUIRE(intention->created_at == 1);
    REQUIRE(story.world().find_in_scene("golden_b", "Alice") != nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Alice") == nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Bob") != nullptr);
    const std::string live = story.render_transcript();
    REQUIRE(live.find("[Fork]") != std::string::npos);
    REQUIRE(live.find("fork from=golden to=golden_b") != std::string::npos);
    REQUIRE(live.find("## Off-stage — golden_b") != std::string::npos);

    World detached = story.world_snapshot();
    Node node;
    node.fact = "A second gate appears";
    detached.graph().add_node(std::move(node));
    REQUIRE(detached.graph().size() == 3);
    REQUIRE(story.observations().size() == 2);
}

TEST_CASE("story: owns stable storyline records", "[substrate][story]") {
    Story story = build_fixture();
    SceneData* parent = story.active_scene();
    REQUIRE(story.fork_scene(
        "golden", "golden_b", {"Alice"}, "Guard the road") != nullptr);
    REQUIRE(story.fork_scene(
        "golden", "golden_b", {"Bob"}, "Scout ahead") == nullptr);
    REQUIRE(story.fork_scene(
        "missing", "golden_c", {"Bob"}, "Scout ahead") == nullptr);
    REQUIRE(story.fork_scene(
        "golden", "golden_c", {}, "Scout ahead") == nullptr);
    REQUIRE(story.fork_scene(
        "golden", "golden_c", {"Unknown"}, "Scout ahead") == nullptr);
    REQUIRE(story.fork_scene(
        "golden", "golden_c", {"Bob"}, "") == nullptr);
    REQUIRE(story.fork_scene(
        "golden", "golden_c", {"Bob"}, "Scout ahead") != nullptr);
    REQUIRE(story.get_scene("golden") == parent);
    REQUIRE(story.scene_count() == 3);
}

TEST_CASE("story: failed fork synthesis changes no domain state",
          "[substrate][fork]") {
    Story story = build_fixture();
    const json world_before = story.world().to_json();
    const std::vector<std::string> ids_before = story.scene_ids();
    story.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&,
           const ReadToolCallback&) { return std::string{"not json"}; });

    REQUIRE(story.fork_scene(
        "golden", "failed", {"Alice"}, "Guard the road") == nullptr);
    REQUIRE(story.scene_ids() == ids_before);
    REQUIRE(story.world().to_json() == world_before);
}

TEST_CASE("story: fork cannot empty its parent scene", "[substrate][fork]") {
    SceneData scene;
    scene.scene_id = "root";
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(std::move(scene), std::move(world));
    bool narrator_called = false;
    story.set_narrator_llm_callback(
        [&](const std::string&, const std::string&, const std::string&,
            const ReadToolCallback&) {
            narrator_called = true;
            return std::string{R"({"fork_story_so_far":"Scout leaves."})"};
        });

    REQUIRE(story.fork_scene(
        "root", "empty", {"Scout"}, "Leave") == nullptr);
    REQUIRE_FALSE(narrator_called);
    REQUIRE(story.world().find_in_scene("root", "Scout") != nullptr);
}

TEST_CASE("story: merge synthesizes context before moving cast and retiring source",
          "[substrate][story]") {
    Story story = build_fixture();
    REQUIRE(story.fork_scene("golden", "hunt", {"Bob"}, "Hunt") != nullptr);
    append_history_message(
        story.get_scene("hunt")->history,
        stamped(Role::Assistant, "Bob follows tracks through the woods.",
                "2026-01-02T00:00:00Z"));
    story.active_scene()->downsampling =
        text_downsampling_from_summary("Alice guards the barred gate.", 0);
    const std::size_t target_history_size = story.active_scene()->history.size();

    bool narrator_called = false;
    story.set_narrator_llm_callback(
        [&](const std::string& scene_id, const std::string& instructions,
            const std::string& turn_state, const ReadToolCallback& read_tool) {
            narrator_called = true;
            REQUIRE(scene_id == "golden");
            REQUIRE(instructions.find("merged_story_so_far") != std::string::npos);
            const json context = json::parse(turn_state);
            REQUIRE(context["source"]["scene_id"] == "hunt");
            REQUIRE(context["destination"]["scene_id"] == "golden");
            REQUIRE(context["source"]["recent_timeline"][0]["content"] ==
                    "Bob follows tracks through the woods.");
            const json history = json::parse(read_tool(
                "query_history", R"({"scene_id":"hunt","query":"tracks"})"));
            REQUIRE(history["snippets"].size() == 1);
            return std::string{
                R"({"merged_story_so_far":"Alice holds the gate as Bob arrives from the woods with the trail unresolved."})"};
        });

    REQUIRE(story.merge_scene("hunt", "golden"));
    REQUIRE(narrator_called);
    REQUIRE(story.get_scene("hunt") == nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Bob") != nullptr);
    REQUIRE(story.active_scene()->history.size() == target_history_size + 1);
    REQUIRE(render_text_downsampling(story.active_scene()->downsampling) ==
            "Alice holds the gate as Bob arrives from the woods with the trail unresolved.");
    const std::string transcript = story.render_transcript();
    REQUIRE(transcript.find("[Fork]") != std::string::npos);
    REQUIRE(transcript.find("fork from=golden to=hunt") != std::string::npos);
    REQUIRE(transcript.find("[Merge]") != std::string::npos);
    REQUIRE(transcript.find("merge from=hunt into=golden") != std::string::npos);
    REQUIRE(transcript.find("## Fork — hunt (merged into golden)") !=
            std::string::npos);
    REQUIRE(transcript.find("Bob follows tracks through the woods.") !=
            std::string::npos);
    REQUIRE(transcript.find("## Off-stage — hunt") == std::string::npos);

    const std::string saves = temp_saves_dir();
    story.save(saves);
    std::ifstream manifest_file(
        std::filesystem::path(saves) / "story.json");
    json manifest;
    manifest_file >> manifest;
    REQUIRE(manifest["scene_closures"].size() == 1);
    REQUIRE(manifest["scene_closures"][0]["scene_id"] == "hunt");
    REQUIRE(manifest["scene_closures"][0]["merged_into"] == "golden");
    REQUIRE(manifest["scene_closures"][0]["cast"] == json::array({"Bob"}));

    SceneData shell;
    shell.scene_id = "golden";
    Story reloaded = Story::from_data(std::move(shell));
    reloaded.load_save(saves);
    REQUIRE(reloaded.scene_count() == 1);
    REQUIRE(render_text_downsampling(reloaded.active_scene()->downsampling) ==
            "Alice holds the gate as Bob arrives from the woods with the trail unresolved.");
    const std::string reloaded_transcript = reloaded.render_transcript();
    REQUIRE(reloaded_transcript.find("## Fork — hunt (merged into golden)") !=
            std::string::npos);
    REQUIRE(reloaded_transcript.find("[Fork]") != std::string::npos);
    REQUIRE(reloaded_transcript.find("[Merge]") != std::string::npos);
}

TEST_CASE("story: failed merge synthesis leaves both scenes unchanged",
          "[substrate][story]") {
    Story story = build_fixture();
    REQUIRE(story.fork_scene("golden", "hunt", {"Bob"}, "Hunt") != nullptr);
    const json target_downsampling =
        downsampling_to_json(story.active_scene()->downsampling);
    story.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&,
           const ReadToolCallback&) { return std::string{"not json"}; });

    REQUIRE_FALSE(story.merge_scene("hunt", "golden"));
    REQUIRE(story.get_scene("hunt") != nullptr);
    REQUIRE(story.world().find_in_scene("hunt", "Bob") != nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Bob") == nullptr);
    REQUIRE(downsampling_to_json(story.active_scene()->downsampling) ==
            target_downsampling);
}

TEST_CASE("story: merge cannot retire the Player storyline",
          "[substrate][story]") {
    Story story = build_fixture();
    REQUIRE(story.fork_scene(
        "golden", "hunt", {"Bob"}, "Hunt") != nullptr);
    bool narrator_called = false;
    story.set_narrator_llm_callback(
        [&](const std::string&, const std::string&, const std::string&,
            const ReadToolCallback&) {
            narrator_called = true;
            return std::string{
                R"({"merged_story_so_far":"This must not be used."})"};
        });

    REQUIRE_FALSE(story.merge_scene("golden", "hunt"));
    REQUIRE_FALSE(narrator_called);
    REQUIRE(story.get_scene("golden") != nullptr);
    REQUIRE(story.world().find_in_scene("golden", "Player") != nullptr);
}

TEST_CASE("story: conclusion expires its intention and persists a compact closure",
          "[substrate][story]") {
    Story story = build_fixture();
    SceneData* child = story.fork_scene(
        "golden", "hunt", {"Bob"}, "Hunt the intruder");
    REQUIRE(child != nullptr);
    append_history_message(
        child->history,
        stamped(Role::Assistant, "Bob corners the intruder and ends the chase.",
                "2026-01-02T00:00:00Z"));
    const std::uint64_t intention_id = child->intention_node_id;
    const std::string saves = temp_saves_dir();
    story.save(saves);
    REQUIRE(std::filesystem::exists(
        std::filesystem::path(saves) / "hunt.json"));

    REQUIRE_FALSE(story.conclude_scene("golden", "cannot retire Player"));
    REQUIRE(story.conclude_scene("hunt", "the chase is over"));
    REQUIRE(story.active_scene_id() == "golden");
    REQUIRE(story.world().find_in_scene("hunt", "Bob") == nullptr);
    const Node* intention = story.world().character_memories().at("Bob")
        .beliefs().get_node(intention_id);
    REQUIRE(intention != nullptr);
    REQUIRE(intention->valid_until == 0);
    REQUIRE_FALSE(story.conclude_scene("golden", "final scene"));

    story.save(saves);
    REQUIRE_FALSE(std::filesystem::exists(
        std::filesystem::path(saves) / "hunt.json"));
    std::ifstream manifest_file(
        std::filesystem::path(saves) / "story.json");
    json manifest;
    manifest_file >> manifest;
    REQUIRE(manifest["scene_closures"].size() == 1);
    REQUIRE(manifest["scene_closures"][0]["scene_id"] == "hunt");
    REQUIRE(manifest["scene_closures"][0]["reason"] == "the chase is over");
    REQUIRE(manifest["scene_closures"][0]["cast"] == json::array({"Bob"}));
    REQUIRE(manifest["scene_closures"][0]["final_narration"] ==
            "Bob corners the intruder and ends the chase.");

    SceneData shell;
    shell.scene_id = "golden";
    Story reloaded = Story::from_data(std::move(shell));
    reloaded.load_save(saves);
    reloaded.save(saves);
    std::ifstream reloaded_manifest_file(
        std::filesystem::path(saves) / "story.json");
    json reloaded_manifest;
    reloaded_manifest_file >> reloaded_manifest;
    REQUIRE(reloaded_manifest["scene_closures"] ==
            manifest["scene_closures"]);
}

TEST_CASE("story: staleness tracks turns", "[substrate][story]") {
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
    append_history_message(
        story.get_scene("hunt")->history,
        stamped(Role::Assistant, "Bob slips into the woods.",
                "2026-01-02T00:00:00Z"));
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
    REQUIRE(story.observations().size() == 2);
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
