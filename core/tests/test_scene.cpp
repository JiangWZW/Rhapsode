#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>
#include <type_traits>

#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/graph_plan.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/node.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/turn_pipeline.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/weaver.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

using namespace rhapsode;
using json = nlohmann::json;

namespace {

SceneData basic_scene(const std::string& id = "root") {
    SceneData scene;
    scene.scene_id = id;
    scene.title = "Root";
    scene.system_prompt = "Narrate.";
    return scene;
}

std::string response(const std::string& prose,
                     const std::string& plan =
                         R"({"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})") {
    return prose + "\n<<<RHAPSODE_JSON>>>\n" + plan;
}

using TestNarratorCallback = std::function<std::string(
    const std::string&, const std::string&, const std::string&)>;

NarratorLLMCallback with_unused_read_tools(TestNarratorCallback narrator) {
    return [narrator = std::move(narrator)](
               const std::string& scene_id, const std::string& instructions,
               const std::string& turn_state, const ReadToolCallback&) {
        return narrator(scene_id, instructions, turn_state);
    };
}

void configure_runtime(TurnServices& services, TestNarratorCallback narrator) {
    services.llm = [](const std::string&) { return std::string{"fallback"}; };
    services.narrator = with_unused_read_tools(std::move(narrator));
}

TurnResult execute_test_turn(
    World& world, TurnServices& services, SceneData& scene,
    const std::string& text,
    TurnInput::Kind kind = TurnInput::Kind::Player) {
    StoryData data;
    import_world(data, world);
    data.active_scene_id = scene.scene_id;
    adopt_scene(data, scene);
    try {
        TurnResult result = execute_turn(
            data, services, {kind, scene.scene_id, text});
        REQUIRE(result.graph_settlement.has_value());
        result.effects = settle_graph_observations(
            data, services, std::move(*result.graph_settlement));
        result.graph_settlement.reset();
        world = snapshot_world(data);
        scene = std::move(*data.scenes.front());
        return result;
    } catch (...) {
        world = snapshot_world(data);
        scene = std::move(*data.scenes.front());
        throw;
    }
}

std::vector<Node> process_test_post_turn(
    World& world, TurnServices& services, SceneData& scene) {
    StoryData data;
    import_world(data, world);
    data.active_scene_id = scene.scene_id;
    adopt_scene(data, scene);
    auto expired = process_post_turn(data, services, scene.scene_id);
    world = snapshot_world(data);
    scene = std::move(*data.scenes.front());
    return expired;
}

void configure_story(Story& story, TestNarratorCallback narrator) {
    story.set_llm_callback([](const std::string&) { return std::string{"fallback"}; });
    story.set_narrator_llm_callback(
        [narrator = std::move(narrator)](
            const std::string& scene_id, const std::string& instructions,
            const std::string& turn_state, const ReadToolCallback&) {
            if (instructions.find("fork_story_so_far") != std::string::npos) {
                return std::string{
                    R"({"fork_story_so_far":"The departing cast carries its established situation into a parallel thread."})"};
            }
            if (instructions.find("merged_story_so_far") != std::string::npos) {
                return std::string{
                    R"({"merged_story_so_far":"The parallel cast reunites with the player storyline."})"};
            }
            return narrator(scene_id, instructions, turn_state);
        });
}

std::vector<SceneMessage> play_turn(Story& story, const std::string& input) {
    auto outs = story.advance_player(input);
    auto more = story.complete_turn();
    outs.insert(outs.end(), more.begin(), more.end());
    return outs;
}

std::filesystem::path temp_dir(const std::string& prefix) {
    const auto path = std::filesystem::temp_directory_path() /
        (prefix + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::remove_all(path);
    return path;
}

std::uint64_t add_fact(WorldGraph& graph, const std::string& fact,
                       const std::string& entity, int turn) {
    Node node;
    node.fact = fact;
    node.entities = {entity};
    node.state = NodeState::Active;
    node.created_at = turn;
    return graph.add_node(std::move(node)).id;
}

}  // namespace

TEST_CASE("SceneData is a World-free aggregate", "[scene_data][ownership]") {
    STATIC_REQUIRE(std::is_aggregate_v<SceneData>);
    STATIC_REQUIRE(std::is_aggregate_v<DownsamplingState>);
    SceneData scene;
    scene.scene_id = "root";
    REQUIRE(scene.scene_id == "root");
    REQUIRE(scene.turn_index == -1);
}

TEST_CASE("Narrator instructions are stage craft with schema last",
          "[narrator_prompt][characterization]") {
    const std::string turn = build_narrator_instructions();
    const auto craft = turn.find("You narrate a live scene");
    const auto sentinel = turn.find("<<<RHAPSODE_JSON>>>");
    const auto schema = turn.find("\"speech_turns\"");
    REQUIRE(craft != std::string::npos);
    REQUIRE(sentinel != std::string::npos);
    REQUIRE(schema != std::string::npos);
    REQUIRE(craft < sentinel);
    REQUIRE(sentinel < schema);
    REQUIRE(turn.find("### Remember") == std::string::npos);
    REQUIRE(turn.find("new_characters") != std::string::npos);
    REQUIRE(turn.find("active_cast") != std::string::npos);

    const std::string graph = build_narrator_graph_instructions();
    REQUIRE(graph.find("GRAPH_UPDATE") != std::string::npos);
    const auto mark = graph.find("GRAPH_UPDATE");
    const auto graph_sentinel = graph.find("<<<RHAPSODE_JSON>>>");
    const auto graph_schema = graph.find("\"new_nodes\"");
    REQUIRE(mark < graph_sentinel);
    REQUIRE(graph_sentinel < graph_schema);
    REQUIRE(graph.find("### Remember") == std::string::npos);
}

TEST_CASE("Narrator turn state puts attributed speech last",
          "[narrator_prompt][characterization]") {
    World world;
    Character aqua{"Aqua", "A loud goddess", false};
    aqua.role = "goddess";
    aqua.dialogue_instructions = "Loud, indignant";
    world.enter_character("root", std::move(aqua));
    world.enter_character("guild", Character{"Kazuma", "The guy", false});

    SceneData scene = basic_scene();
    scene.downsampling = text_downsampling_from_summary(
        "Yesterday the party left the guild.", 0);

    SceneMessage prior_player;
    prior_player.role = Role::User;
    prior_player.content = "I insult Aqua.";
    prior_player.metadata = {
        {"scene_kind", "player"}, {"turn", 1}, {"turn_ordinal", 0}};
    append_history_message(scene.history, prior_player);

    SceneMessage prior_narrator;
    prior_narrator.role = Role::Assistant;
    prior_narrator.content = "Aqua bristles.";
    prior_narrator.metadata = {
        {"scene_kind", "narrator"}, {"turn", 1}, {"turn_ordinal", 1}};
    append_history_message(scene.history, prior_narrator);

    SceneMessage spoken;
    spoken.role = Role::Assistant;
    spoken.content = "How dare you!";
    spoken.metadata = {
        {"scene_kind", "character"}, {"speaker", "Aqua"},
        {"turn", 1}, {"turn_ordinal", 2}};
    append_history_message(scene.dialogue, spoken);

    SceneMessage now;
    now.role = Role::User;
    now.content = "I do it again.";
    now.metadata = {
        {"scene_kind", "player"}, {"turn", 2}, {"turn_ordinal", 0}};
    append_history_message(scene.history, now);

    const std::string board =
        "Other live threads:\n- This scene (root): you are here. With Aqua.\n";
    const std::string state = build_narrator_turn_state(scene, world, board);

    const auto on_at = state.find("On this stage");
    const auto voice_at = state.find("Loud, indignant");
    const auto off_at = state.find("Not on this stage");
    const auto threads_at = state.find("Other live threads");
    const auto happened_at = state.find("What has already happened");
    const auto beat_at = state.find("What was just said and done");
    const auto line_at = state.find("How dare you!");
    const auto player_at = state.find("Player: I do it again.");
    REQUIRE(on_at != std::string::npos);
    REQUIRE(voice_at != std::string::npos);
    REQUIRE(off_at != std::string::npos);
    REQUIRE(threads_at != std::string::npos);
    REQUIRE(happened_at != std::string::npos);
    REQUIRE(beat_at != std::string::npos);
    REQUIRE(line_at != std::string::npos);
    REQUIRE(player_at != std::string::npos);
    REQUIRE(on_at < voice_at);
    REQUIRE(voice_at < off_at);
    REQUIRE(off_at < threads_at);
    REQUIRE(threads_at < happened_at);
    REQUIRE(happened_at < beat_at);
    REQUIRE(beat_at < line_at);
    REQUIRE(line_at < player_at);
    REQUIRE(state.find("Kazuma") != std::string::npos);
    REQUIRE(state.find("### Remember") == std::string::npos);
    REQUIRE(state.find("user:") == std::string::npos);
    REQUIRE(state.find("assistant:") == std::string::npos);
}

TEST_CASE("Graph narrator user is this take only",
          "[narrator_prompt][two_phase]") {
    World world;
    world.enter_character("root", Character{"Guard", "Alert", false});
    SceneData scene = basic_scene();
    TurnServices runtime;
    std::string graph_state;
    configure_runtime(runtime,
        [&](const std::string&, const std::string& instructions,
            const std::string& turn_state) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                graph_state = turn_state;
                return std::string{
                    "<<<RHAPSODE_JSON>>>\n"
                    R"({"transitions":[],"new_nodes":[{"fact":"A hinge loosens","type":"scene","state":"active","entities":["Gate"]}]})"};
            }
            return response(
                "Wind catches the gate.",
                R"({"speech_turns":[{"character":"Guard","line":"Hold.","action":"raises a hand"}],"new_characters":[],"active_cast":["Guard"]})");
        });

    execute_test_turn(world, runtime, scene, "Listen.");
    REQUIRE(graph_state.find("This take:") != std::string::npos);
    REQUIRE(graph_state.find("Wind catches the gate.") != std::string::npos);
    REQUIRE(graph_state.find("Guard: Hold. (raises a hand)") != std::string::npos);
    REQUIRE(graph_state.find("On this stage: Guard") != std::string::npos);
    REQUIRE(graph_state.find("What was just said") == std::string::npos);
    REQUIRE(graph_state.find("Listen.") == std::string::npos);
}

TEST_CASE("Turn pipeline merges graph observation ops into the turn plan",
          "[turn_pipeline][narrator][two_phase]") {
    World world;
    SceneData scene = basic_scene();
    TurnServices runtime;
    int calls = 0;
    configure_runtime(runtime,
        [&](const std::string&, const std::string& instructions, const std::string&) {
            ++calls;
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                REQUIRE(calls == 2);
                return std::string{
                    "<<<RHAPSODE_JSON>>>\n"
                    R"({"transitions":[],"new_nodes":[{"fact":"A hinge loosens","type":"scene","state":"active","entities":["Gate"]}]})"};
            }
            REQUIRE(calls == 1);
            return response(
                "Wind catches the gate.",
                R"({"speech_turns":[],"new_characters":[],"active_cast":[],"transitions":[],"new_nodes":[]})");
        });

    const TurnResult result = execute_test_turn(
        world, runtime, scene, "Listen.");
    REQUIRE(calls == 2);
    REQUIRE(result.outputs.front().content == "Wind catches the gate.");
    REQUIRE(world.graph().size() == 1);
    REQUIRE(world.graph().all_nodes().front().fact == "A hinge loosens");
}

TEST_CASE("SceneMessage JSON round-trip", "[scene_message]") {
    SceneMessage original;
    original.role = Role::Assistant;
    original.content = "Hello";
    original.timestamp = "2026-01-01T00:00:00Z";
    original.metadata = {{"speaker", "Guard"}};
    const json encoded = original;
    const SceneMessage restored = encoded.get<SceneMessage>();
    REQUIRE(restored.role == original.role);
    REQUIRE(restored.content == original.content);
    REQUIRE(restored.timestamp == original.timestamp);
    REQUIRE(restored.metadata == original.metadata);
}

TEST_CASE("SceneMessage roles serialize correctly", "[scene_message]") {
    for (const auto& [role, name] : std::vector<std::pair<Role, std::string>>{
             {Role::System, "system"}, {Role::User, "user"},
             {Role::Assistant, "assistant"}}) {
        SceneMessage message;
        message.role = role;
        message.content = "x";
        const json value = message;
        REQUIRE(value["role"] == name);
        REQUIRE(value.get<SceneMessage>().role == role);
    }
}

TEST_CASE("Scene history append, snapshot, truncate, and round-trip", "[history]") {
    std::vector<SceneMessage> history;
    for (int i = 0; i < 4; ++i) {
        SceneMessage message;
        message.role = i % 2 ? Role::Assistant : Role::User;
        message.content = std::to_string(i);
        append_history_message(history, std::move(message));
    }
    REQUIRE(history.size() == 4);
    REQUIRE(snapshot_history(history, 2).front().content == "2");
    REQUIRE_FALSE(history.front().timestamp.empty());
    truncate_history(history, 3);
    REQUIRE(history.size() == 3);
    const json value = history;
    REQUIRE(history_from_json(value).size() == 3);
}

TEST_CASE("Turn take is this turn's narrator and speech",
          "[history][characterization]") {
    SceneData scene = basic_scene();
    SceneMessage player;
    player.role = Role::User;
    player.content = "go";
    player.metadata = {{"scene_kind", "player"}, {"turn", 1}};
    append_history_message(scene.history, std::move(player));
    SceneMessage narrator;
    narrator.role = Role::Assistant;
    narrator.content = "The gate swings.";
    narrator.metadata = {{"scene_kind", "narrator"}, {"turn", 1}};
    append_history_message(scene.history, std::move(narrator));
    SceneMessage speech;
    speech.role = Role::Assistant;
    speech.content = "Hold.";
    speech.metadata = {
        {"scene_kind", "character"}, {"speaker", "Scout"}, {"turn", 1}};
    append_history_message(scene.dialogue, std::move(speech));
    SceneMessage later;
    later.role = Role::Assistant;
    later.content = "Next take.";
    later.metadata = {{"scene_kind", "narrator"}, {"turn", 2}};
    append_history_message(scene.history, std::move(later));

    const std::string take = format_turn_take(scene, 1);
    REQUIRE(take.find("The gate swings.") != std::string::npos);
    REQUIRE(take.find("Scout: Hold.") != std::string::npos);
    REQUIRE(take.find("go") == std::string::npos);
    REQUIRE(take.find("Next take.") == std::string::npos);
}

TEST_CASE("Narration window keeps the last n turns and the suffix cap",
          "[history][characterization]") {
    SceneData scene = basic_scene();
    auto add_narrator = [&](int turn, const std::string& text) {
        SceneMessage message;
        message.role = Role::Assistant;
        message.content = text;
        message.metadata = {{"scene_kind", "narrator"}, {"turn", turn}};
        append_history_message(scene.history, std::move(message));
    };
    SceneMessage player;
    player.role = Role::User;
    player.content = "go";
    player.metadata = {{"scene_kind", "player"}, {"turn", 1}};
    append_history_message(scene.history, std::move(player));
    add_narrator(1, "Turn one oldest.");
    add_narrator(2, "Turn two.");
    add_narrator(3, "Turn three.");
    add_narrator(4, "Turn four newest.");

    const std::string window = format_narration_window(scene, 4, 3, 1800);
    REQUIRE(window.find("Turn one oldest.") == std::string::npos);
    REQUIRE(window.find("go") == std::string::npos);
    REQUIRE(window.find("Turn two.") != std::string::npos);
    REQUIRE(window.find("Turn three.") != std::string::npos);
    REQUIRE(window.find("Turn four newest.") != std::string::npos);

    const std::string capped = format_narration_window(scene, 4, 3, 18);
    REQUIRE(capped.find("Turn two.") == std::string::npos);
    REQUIRE(capped.find("newest") != std::string::npos);
}

TEST_CASE("Scene history drops messages from a reverted turn", "[history]") {
    std::vector<SceneMessage> history;
    for (int turn : {0, 1, 2}) {
        SceneMessage message;
        message.role = Role::Assistant;
        message.content = std::to_string(turn);
        message.metadata["turn"] = turn;
        append_history_message(history, std::move(message));
    }
    drop_history_from_turn(history, 1);
    REQUIRE(history.size() == 1);
    REQUIRE(history.front().content == "0");
}

TEST_CASE("Text downsampling state preserves processing and JSON",
          "[downsampling]") {
    DownsamplingState state;
    std::vector<SceneMessage> messages;
    for (int index = 0; index < 9; ++index) {
        SceneMessage message;
        message.role = index % 2 ? Role::Assistant : Role::User;
        message.content = std::to_string(index);
        messages.push_back(std::move(message));
    }

    int calls = 0;
    process_text_downsampling(
        state, messages,
        [&](const std::string&) {
            ++calls;
            return std::string{"  first summary  "};
        });

    REQUIRE(calls == 1);
    REQUIRE(state.summarized_up_to == 3);
    REQUIRE(state.levels[0].snippets.size() == 1);
    REQUIRE(render_text_downsampling(state) == "first summary");

    const auto value = downsampling_to_json(state);
    const auto restored = downsampling_from_json(value);
    REQUIRE(downsampling_to_json(restored) == value);
}

TEST_CASE("Character JSON round-trip preserves membership", "[character]") {
    Character character{"Scout", "Careful", false};
    character.join_scene("root");
    character.role = "companion";
    const json value = character;
    const Character restored = value.get<Character>();
    REQUIRE(restored.name == "Scout");
    REQUIRE(restored.in_scene("root"));
    REQUIRE(restored.role == "companion");
}

TEST_CASE("Story scenario JSON builds World and SceneData", "[story][scenario]") {
    const json scenario = {
        {"title", "Hall"},
        {"system_prompt", "Narrate the hall."},
        {"characters", json::array({
            {{"name", "Player"}, {"description", "You"}, {"is_player", true}},
            {{"name", "Guard"}, {"description", "Alert"}, {"on_stage", true}}
        })},
        {"seed_messages", json::array({
            {{"role", "assistant"}, {"content", "The hall waits."}}
        })}
    };
    Story story = Story::from_scenario_json(scenario, "hall");
    REQUIRE(story.active_scene()->title == "Hall");
    REQUIRE(story.active_scene()->history.size() == 1);
    REQUIRE(story.world().characters().size() == 2);
    REQUIRE(story.world().find_in_scene("hall", "Guard") != nullptr);
    REQUIRE(story.world().character_memories().count("Guard") == 1);
    REQUIRE(story.to_scenario_json("hall")["title"] == "Hall");
}

TEST_CASE("WorldGraph DOT and JSON retain graph structure", "[world_graph]") {
    WorldGraph graph;
    const auto first = add_fact(graph, "Gate shut", "Gate", 1);
    const auto second = add_fact(graph, "Gate open", "Gate", 2);
    REQUIRE(graph.add_relation(first, second, 0.75f, 2, "supersedes"));
    const std::string dot = graph.to_dot();
    REQUIRE(dot.find("digraph WorldGraph") != std::string::npos);
    REQUIRE(dot.find("Gate shut") != std::string::npos);
    const WorldGraph restored = WorldGraph::from_json(graph.to_json());
    REQUIRE(restored.size() == 2);
    REQUIRE(restored.all_edges().size() == 1);
}

TEST_CASE("World owns roster and membership mutations", "[world]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    REQUIRE(world.find_in_scene("root", "Scout") != nullptr);
    world.move_scene_members("root", "ridge", {"Scout"});
    REQUIRE(world.find_in_scene("root", "Scout") == nullptr);
    REQUIRE(world.find_in_scene("ridge", "Scout") != nullptr);
    REQUIRE(world.leave_character("ridge", "Scout"));
    REQUIRE(world.find_in_scene("ridge", "Scout") == nullptr);
}

TEST_CASE("World death mutation clears all membership", "[world]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    world.move_scene_members("root", "ridge", {"Scout"});
    REQUIRE(world.mark_character_dead("Scout"));
    const Character* scout = world.find_character("Scout");
    REQUIRE(scout != nullptr);
    REQUIRE(scout->dead);
    REQUIRE(scout->scene_ids.empty());
}

TEST_CASE("New World memories accept call-scoped monologue configuration",
          "[world][character_memory]") {
    World world;
    int calls = 0;
    const LLMCallback monologue = [&](const std::string&) {
        ++calls;
        return std::string{"not json"};
    };
    world.enter_character("root", Character{"Scout", "Careful", false});
    world.update_monologues("root", 2, monologue);
    REQUIRE(calls == 1);
}

TEST_CASE("Loaded World memories accept call-scoped monologue configuration",
          "[world][character_memory][persistence]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    int calls = 0;
    const LLMCallback monologue = [&](const std::string&) {
        ++calls;
        return std::string{"not json"};
    };
    world = World::from_json(world.to_json());
    world.update_monologues("root", 2, monologue);
    REQUIRE(calls == 1);
}

TEST_CASE("World state version defaults for old saves and round-trips",
          "[world][version][persistence]") {
    json old_save = World{}.to_json();
    old_save.erase("state_version");
    World restored_old = World::from_json(old_save);
    REQUIRE(restored_old.state_version() == 0);
    REQUIRE(restored_old.advance_state_version() == 1);

    World current;
    REQUIRE(current.advance_state_version() == 1);
    REQUIRE(current.advance_state_version() == 2);
    World restored_current = World::from_json(current.to_json());
    REQUIRE(restored_current.state_version() == 2);
    REQUIRE(restored_current.advance_state_version() == 3);
}

TEST_CASE("On-stage minds each overwrite their own perception",
          "[world][memory]") {
    SceneData scene = basic_scene();
    SceneMessage narrator;
    narrator.role = Role::Assistant;
    narrator.content = "The bell rings.";
    narrator.metadata = {{"scene_kind", "narrator"}, {"turn", 1}};
    append_history_message(scene.history, std::move(narrator));
    const std::string window = format_narration_window(
        scene, 1, CharacterMemory::kPerceptionWindowTurns,
        CharacterMemory::kPerceptionUserChars);

    World world;
    world.enter_character("root", Character{"Alice", "A", false});
    world.enter_character("root", Character{"Bob", "B", false});
    world.update_perceptions("root", 1, window,
        [](const std::string& prompt) {
            if (prompt.find("You are Alice") != std::string::npos)
                return std::string{R"({"perception":"Alice hears the bell."})"};
            return std::string{R"({"perception":"Bob notices the bell."})"};
        });

    const auto& alice = world.character_memories().at("Alice");
    const auto& bob = world.character_memories().at("Bob");
    REQUIRE(alice.perception() == "Alice hears the bell.");
    REQUIRE(bob.perception() == "Bob notices the bell.");
    REQUIRE(alice.perception_turn() == 1);
    REQUIRE(bob.perception_turn() == 1);

    std::string alice_prompt;
    std::string bob_prompt;
    world.update_monologues("root", 1,
        [&](const std::string& prompt) {
            if (prompt.find("You are Alice") != std::string::npos)
                alice_prompt = prompt;
            else
                bob_prompt = prompt;
            return std::string{R"({"line":null})"};
        });
    REQUIRE(alice_prompt.find("Alice hears the bell.") != std::string::npos);
    REQUIRE(alice_prompt.find("Bob notices the bell.") == std::string::npos);
    REQUIRE(alice_prompt.find("[1 take]") == std::string::npos);
    REQUIRE(bob_prompt.find("Bob notices the bell.") != std::string::npos);
    REQUIRE(bob_prompt.find("Alice hears the bell.") == std::string::npos);

    World restored = World::from_json(world.to_json());
    REQUIRE(restored.character_memories().at("Alice").perception()
            == "Alice hears the bell.");
    REQUIRE(restored.character_memories().at("Bob").perception()
            == "Bob notices the bell.");
}

TEST_CASE("Turn services retain no graph identity",
          "[turn_pipeline][ownership]") {
    Weaver weaver;
    WorldGraph first;
    WorldGraph second;
    weaver.set_interval(1);
    REQUIRE(weaver.should_weave(0));
    REQUIRE(weaver.weave(first, 0).analysis.live_node_count == 0);
    REQUIRE(weaver.weave(second, 0).analysis.live_node_count == 0);
}

TEST_CASE("Weaver activation preserves optional runtime semantics",
          "[weaver][ownership]") {
    WorldGraph graph;
    Weaver weaver;
    REQUIRE_FALSE(weaver.active());
    weaver.set_interval(3);
    REQUIRE(weaver.active());
}

TEST_CASE("Turn pipeline returns associated generic turn effects",
          "[turn_pipeline][result][post_turn]") {
    World world;
    SceneData scene = basic_scene("root");
    const auto old_id = add_fact(world.graph(), "The gate is closed", "Gate", 1);
    const auto new_id = add_fact(world.graph(), "The gate is open", "Gate", 5);
    TurnServices runtime;
    Weaver& weaver = runtime.weaver;
    configure_runtime(runtime, [](const std::string&, const std::string&, const std::string&) {
        return response("The gate groans.",
            R"({"transitions":[],"new_nodes":[{"fact":"Wind crosses the gate","entities":["Gate"]}],"speech_turns":[],"new_characters":[],"active_cast":[]})");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback([&](const std::string& prompt) {
        if (prompt.find("NO LONGER TRUE") != std::string::npos) {
            return std::string{"{\"results\":[{\"group_id\":\"g0\","
                "\"superseded\":[{\"id\":"} +
                std::to_string(old_id) + ",\"by\":" + std::to_string(new_id) +
                R"(}],"reason":"newer state"}]})";
        }
        return std::string{"{\"connect\":[{\"from\":"} +
            std::to_string(old_id) + ",\"to\":" + std::to_string(new_id) +
            R"(,"weight":0.8,"reason":"state"}],"disconnect":[],"reweight":[]})";
    });

    const TurnResult result = execute_test_turn(
        world, runtime, scene, "Look.");
    REQUIRE(result.scene_id == "root");
    REQUIRE(scene.turn_index == 0);
    REQUIRE(result.effects.created_nodes.size() == 1);
    REQUIRE(result.effects.expired_nodes.empty());

    const auto expired = process_test_post_turn(
        world, runtime, scene);
    REQUIRE(expired.size() == 1);
    REQUIRE(expired.front().id == old_id);
    const auto edges = world.graph().all_edges();
    REQUIRE(std::any_of(edges.begin(), edges.end(), [&](const EdgeInfo& edge) {
        return edge.from_id == old_id && edge.to_id == new_id &&
               edge.data.active && edge.data.weight == 0.8f;
    }));
}

TEST_CASE("Turn pipeline keeps narrator and dialogue histories separate", "[turn_pipeline]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Guard", "Alert", false});
    TurnServices runtime;
    configure_runtime(runtime, [](const std::string&, const std::string&, const std::string&) {
        return response("The guard raises a hand.",
            R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Guard","line":"Stop.","action":"blocks the door"}],"new_characters":[],"active_cast":["Guard"]})");
    });
    const auto result = execute_test_turn(
        world, runtime, scene, "Approach.");
    REQUIRE(result.outputs.size() == 2);
    REQUIRE(world.state_version() == 1);
    REQUIRE(scene.history.size() == 2);
    REQUIRE(scene.dialogue.size() == 1);
    REQUIRE(scene.dialogue.front().content == "Stop. (blocks the door)");
    REQUIRE(scene.history[0].metadata["scene_kind"] == "player");
    REQUIRE(scene.history[0].metadata["turn"] == 0);
    REQUIRE(scene.history[0].metadata["turn_ordinal"] == 0);
    REQUIRE(scene.history[1].metadata["scene_kind"] == "narrator");
    REQUIRE(scene.history[1].metadata["turn_ordinal"] == 1);
    REQUIRE(scene.dialogue[0].metadata["speaker"] == "Guard");
    REQUIRE(scene.dialogue[0].metadata["turn_ordinal"] == 2);
    REQUIRE(scene.history[0].metadata["message_ref"] == "root:v1:player:0");
    REQUIRE(scene.history[1].metadata["message_ref"] == "root:v1:narrator:1");
    REQUIRE(scene.dialogue[0].metadata["message_ref"] ==
            "root:v1:character:2");

    execute_test_turn(world, runtime, scene, "Again.");
    REQUIRE(world.state_version() == 2);
    REQUIRE(scene.history[2].metadata["message_ref"] == "root:v2:player:0");
}

TEST_CASE("Turn pipeline active_cast adds presence without ejecting cast", "[turn_pipeline]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Alice", "A", false});
    world.enter_character("elsewhere", Character{"Bob", "B", false});
    TurnServices runtime;
    configure_runtime(runtime, [](const std::string&, const std::string&, const std::string&) {
        return response("Bob arrives.",
            R"({"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Bob"]})");
    });
    execute_test_turn(world, runtime, scene, "Wait.");
    REQUIRE(world.find_in_scene("root", "Alice") != nullptr);
    REQUIRE(world.find_in_scene("root", "Bob") != nullptr);
}

TEST_CASE("Turn pipeline retries invalid Player speech without leaking state",
          "[turn_pipeline][transaction]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Guard", "Alert", false});
    TurnServices runtime;
    int calls = 0;
    configure_runtime(runtime, [&](const std::string&, const std::string&, const std::string&) {
        ++calls;
        if (calls == 1)
            return response("Wrong.",
                R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Player","line":"No"}],"new_characters":[],"active_cast":["Guard"]})");
        return response("Corrected.");
    });
    const auto result = execute_test_turn(
        world, runtime, scene, "Act.");
    REQUIRE(calls == 3);  // turn failure + turn retry + graph
    REQUIRE(result.outputs.front().content == "Corrected.");
    REQUIRE(scene.turn_index == 0);
}

TEST_CASE("Turn pipeline rolls back failed turns and can be reused",
          "[turn_pipeline][transaction]") {
    World world;
    SceneData scene = basic_scene();
    TurnServices runtime;
    configure_runtime(runtime,
        [](const std::string&, const std::string&, const std::string&) -> std::string {
            throw std::runtime_error("narrator unavailable");
        });
    REQUIRE_THROWS_AS(
        execute_test_turn(world, runtime, scene, "Act."),
        std::runtime_error);
    REQUIRE(scene.turn_index == -1);
    REQUIRE(scene.history.size() == 0);
    REQUIRE(world.graph().size() == 0);

    runtime.narrator =
        [](const std::string&, const std::string&, const std::string&,
           const ReadToolCallback&) { return response("Recovered."); };
    REQUIRE(execute_test_turn(
        world, runtime, scene, "Again.")
        .outputs.front().content == "Recovered.");
}

TEST_CASE("Failed turns publish no staged output callbacks",
          "[turn_pipeline][transaction][delivery]") {
    World world;
    world.enter_character("root", Character{"Guard", "Alert", false});
    SceneData scene = basic_scene();
    TurnServices runtime;
    runtime.llm =
        [](const std::string&) { return std::string{"fallback"}; };
    runtime.narrator =
        [&](const std::string&, const std::string& instructions,
            const std::string&, const ReadToolCallback&) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                return response("", R"({"transitions":[],"new_nodes":[]})");
            }
            world.advance_state_version();
            throw std::runtime_error("concurrent mutation");
        };
    int delivered = 0;
    runtime.turn_complete = [&](const SceneMessage&) { ++delivered; };

    REQUIRE_THROWS_AS(
    execute_test_turn(
            world, runtime, scene, "Approach."),
        std::runtime_error);
    REQUIRE(delivered == 0);
    REQUIRE(scene.history.empty());
    REQUIRE(scene.dialogue.empty());
    REQUIRE(world.state_version() == 0);
}

TEST_CASE("Graph observation failure cannot roll back a committed turn",
          "[turn_pipeline][transaction][observation]") {
    World world;
    SceneData scene = basic_scene();
    TurnServices runtime;
    std::vector<std::string> events;
    runtime.llm = [](const std::string&) { return std::string{"fallback"}; };
    runtime.narrator =
        [&](const std::string&, const std::string& instructions,
            const std::string&, const ReadToolCallback&) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                events.push_back("observe");
                throw std::runtime_error("extractor unavailable");
            }
            events.push_back("narrate");
            return response("The gate opens.");
        };
    runtime.turn_complete =
        [&](const SceneMessage&) { events.push_back("deliver"); };

    const TurnResult result = execute_test_turn(
        world, runtime, scene, "Open it.");

    REQUIRE(events == std::vector<std::string>{
        "narrate", "deliver", "observe"});
    REQUIRE(result.outputs.front().content == "The gate opens.");
    REQUIRE(result.effects.created_nodes.empty());
    REQUIRE(world.state_version() == 1);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 2);
}

TEST_CASE("Graph observations cannot mark a character dead",
          "[turn_pipeline][observation][authority]") {
    World world;
    world.enter_character("root", Character{"Guard", "Alert", false});
    SceneData scene = basic_scene();
    TurnServices runtime;
    int fallback_calls = 0;
    runtime.llm = [&](const std::string&) {
        ++fallback_calls;
        return std::string{"yes"};
    };
    runtime.narrator =
        [](const std::string&, const std::string& instructions,
           const std::string&, const ReadToolCallback&) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                return response(
                    "", R"({"transitions":[],"new_nodes":[{"fact":"Guard dies","type":"character","state":"active","entities":["Guard"]}]})");
            }
            return response("The guard falls still.");
        };

    execute_test_turn(world, runtime, scene, "Strike.");

    REQUIRE(world.find_character("Guard") != nullptr);
    REQUIRE_FALSE(world.find_character("Guard")->dead);
    REQUIRE(fallback_calls == 0);
}

TEST_CASE("Output callback failure is a post-commit delivery failure",
          "[turn_pipeline][transaction][delivery]") {
    World world;
    world.enter_character("root", Character{"Guard", "Alert", false});
    SceneData scene = basic_scene();
    TurnServices runtime;
    configure_runtime(runtime,
        [](const std::string&, const std::string&, const std::string&) {
            return response("The guard raises a hand.",
                R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Guard","line":"Stop."}],"new_characters":[],"active_cast":[]})");
    });
    int attempts = 0;
    runtime.turn_complete =
        [&](const SceneMessage&) {
            ++attempts;
            throw std::runtime_error("socket unavailable");
        };

    const TurnResult result = execute_test_turn(
        world, runtime, scene, "Approach.");
    REQUIRE(result.outputs.size() == 2);
    REQUIRE(result.delivery_failures ==
            std::vector<std::string>{"socket unavailable", "socket unavailable"});
    REQUIRE(attempts == 2);
    REQUIRE(world.state_version() == 1);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 2);
    REQUIRE(scene.dialogue.size() == 1);
}

TEST_CASE("Turn pipeline autonomous turns remain associated with their SceneData",
          "[turn_pipeline][result]") {
    World world;
    SceneData first = basic_scene("first");
    SceneData second = basic_scene("second");
    TurnServices runtime;
    configure_runtime(runtime,
        [](const std::string& id, const std::string&, const std::string&) {
            return response("Narration for " + id + ".");
        });
    REQUIRE(execute_test_turn(
        world, runtime, first, "Act.").scene_id == "first");
    const auto result = execute_test_turn(
        world, runtime, second, "Continue.",
        TurnInput::Kind::Autonomous);
    REQUIRE(result.scene_id == "second");
    REQUIRE(result.outputs.front().content == "Narration for second.");
}

TEST_CASE("Turn pipeline blocks until post-turn processing finishes",
          "[turn_pipeline][post_turn]") {
    using namespace std::chrono_literals;
    World world;
    SceneData scene = basic_scene();
    add_fact(world.graph(), "Gate shut", "Gate", 1);
    add_fact(world.graph(), "Torch lit", "Torch", 1);
    TurnServices runtime;
    Weaver& weaver = runtime.weaver;
    configure_runtime(runtime, [](const std::string&, const std::string&, const std::string&) {
        return response("Time passes.");
    });
    weaver.set_interval(1);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });

    const TurnResult turn = execute_test_turn(
        world, runtime, scene, "Wait.");
    REQUIRE(scene.turn_index == 0);

    auto running = std::async(std::launch::async, [&] {
        return process_test_post_turn(
            world, runtime, scene);
    });
    started.get_future().wait();
    REQUIRE(running.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(running.wait_for(2s) == std::future_status::ready);
    REQUIRE_NOTHROW(running.get());
}

TEST_CASE("Turn pipeline preserves post-turn callback order",
          "[turn_pipeline][post_turn][characterization]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Scout", "Careful", false});
    add_fact(world.graph(), "The gate is closed", "Gate", 1);
    add_fact(world.graph(), "The gate is open", "Gate", 2);
    for (int i = 0; i < 7; ++i) {
        SceneMessage prior;
        prior.role = i % 2 ? Role::Assistant : Role::User;
        prior.content = "Prior " + std::to_string(i);
        append_history_message(scene.history, std::move(prior));
    }

    std::vector<std::string> events;
    TurnServices runtime;
    Weaver& weaver = runtime.weaver;
    configure_runtime(runtime, [&](const std::string&, const std::string&,
                             const std::string&) {
        events.push_back("narrator");
        return response("Wind crosses the gate.",
            R"({"transitions":[],"new_nodes":[{"fact":"Wind crosses the gate","entities":["Gate"]}],"speech_turns":[],"new_characters":[],"active_cast":["Scout"]})");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback([&](const std::string& prompt) {
        if (prompt.find("NO LONGER TRUE") != std::string::npos) {
            events.push_back("expiry");
            return std::string{
                R"({"results":[{"group_id":"g0","superseded":[],"reason":"current"}]})"};
        }
        events.push_back("weave");
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });
    runtime.reflection = [&](const std::string&) {
        events.push_back("reflection");
        return std::string{R"({"line":null})"};
    };
    runtime.downsampler = [&](const std::string&) {
        events.push_back("downsample");
        return std::string{"summary"};
    };

    const TurnResult turn = execute_test_turn(
        world, runtime, scene, "Look.");
    REQUIRE(events == std::vector<std::string>{"narrator", "narrator"});
    process_test_post_turn(world, runtime, scene);
    REQUIRE(events == std::vector<std::string>{
        "narrator", "narrator", "weave", "expiry", "reflection", "downsample"});
}

TEST_CASE("Turn pipeline post-turn failures remain non-fatal",
          "[turn_pipeline][post_turn][characterization]") {
    World world;
    SceneData scene = basic_scene();
    add_fact(world.graph(), "Gate shut", "Gate", 1);
    add_fact(world.graph(), "Torch lit", "Torch", 1);
    TurnServices runtime;
    Weaver& weaver = runtime.weaver;
    configure_runtime(runtime, [](const std::string&, const std::string&,
                            const std::string&) {
        return response("Time passes.");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback(
        [](const std::string&) -> std::string {
            throw std::runtime_error("weaver unavailable");
        });

    const TurnResult result = execute_test_turn(
        world, runtime, scene, "Wait.");
    REQUIRE(result.outputs.front().content == "Time passes.");
    REQUIRE(scene.turn_index == 0);
    REQUIRE_NOTHROW(process_test_post_turn(
        world, runtime, scene));
}

TEST_CASE("Story delivers player outputs before graph settlement and weave",
           "[story][advance_player][post_turn]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    add_fact(world.graph(), "Gate shut", "Gate", 1);
    add_fact(world.graph(), "Torch lit", "Torch", 1);
    Story story = Story::from_data(basic_scene(), std::move(world));
    bool graph_ran = false;
    configure_story(story, [&](const std::string&, const std::string& instructions,
                               const std::string&) {
        if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
            graph_ran = true;
            return response("",
                R"({"transitions":[],"new_nodes":[{"fact":"A bell rings","entities":["Bell"]}]})");
        }
        return response("Time passes.");
    });
    story.set_weaver_interval(1);
    bool weave_ran = false;
    story.set_weaver_llm_callback([&](const std::string&) {
        REQUIRE(graph_ran);
        weave_ran = true;
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });

    const auto graph_size = story.observations().size();
    const auto outputs = story.advance_player("Wait.");
    REQUIRE(outputs.front().content == "Time passes.");
    REQUIRE_FALSE(graph_ran);
    REQUIRE_FALSE(weave_ran);
    REQUIRE(story.observations().size() == graph_size);
    const auto more = story.complete_turn();
    REQUIRE(graph_ran);
    REQUIRE(weave_ran);
    REQUIRE(story.observations().size() == graph_size + 1);
    REQUIRE(more.empty());
}

TEST_CASE("Stale graph settlement is discarded before its callback",
          "[turn_pipeline][observation][version]") {
    StoryData data;
    data.active_scene_id = "root";
    adopt_scene(data, basic_scene());
    TurnServices services;
    int graph_calls = 0;
    services.llm = [](const std::string&) { return std::string{"fallback"}; };
    services.narrator =
        [&](const std::string&, const std::string& instructions,
            const std::string&, const ReadToolCallback&) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                ++graph_calls;
                return response("", R"({"transitions":[],"new_nodes":[]})");
            }
            return response("The room remains still.");
        };

    TurnResult result = execute_turn(
        data, services, {TurnInput::Kind::Player, "root", "Wait."});
    REQUIRE(result.graph_settlement.has_value());
    ++data.transaction_version;

    const auto effects = settle_graph_observations(
        data, services, std::move(*result.graph_settlement));
    REQUIRE(graph_calls == 0);
    REQUIRE(effects.created_nodes.empty());
    REQUIRE(effects.expired_nodes.empty());
}

TEST_CASE("Turn execution reads one frozen transaction version",
          "[turn_pipeline][read_tools][version]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    StoryData data;
    import_world(data, std::move(world));
    data.active_scene_id = "root";
    adopt_scene(data, basic_scene());
    TurnServices services;
    services.llm =
        [](const std::string&) { return std::string{"fallback"}; };

    bool checked_snapshot = false;
    bool checked_graph_snapshot = false;
    int turn_calls = 0;
    REQUIRE(data.transaction_version == 0);
    services.narrator =
        [&](const std::string&, const std::string& instructions,
            const std::string&, const ReadToolCallback& read_tool) {
            if (instructions.find("GRAPH_UPDATE") != std::string::npos) {
                const std::string graph_read =
                    read_tool("query_graph", R"({"query":"LateArrival"})");
                REQUIRE(graph_read.find("LateArrival") == std::string::npos);
                checked_graph_snapshot = true;
                return response("", R"({"transitions":[],"new_nodes":[]})");
            }

            ++turn_calls;
            REQUIRE(data.transaction_version == 0);
            const std::string before =
                read_tool("query_graph", R"({"query":"LateArrival"})");
            Node late;
            late.fact = "LateArrival entered after the snapshot";
            late.entities = {"LateArrival"};
            data.observations.add_node_chained(std::move(late), 0);
            ++data.transaction_version;
            const std::string after =
                read_tool("query_graph", R"({"query":"LateArrival"})");
            REQUIRE(after == before);
            checked_snapshot = true;
            return response("The room remains still.");
        };

    TurnResult result = execute_turn(
        data, services, {TurnInput::Kind::Player, "root", "Wait."});
    REQUIRE(result.graph_settlement.has_value());
    result.effects = settle_graph_observations(
        data, services, std::move(*result.graph_settlement));
    REQUIRE(checked_snapshot);
    REQUIRE(checked_graph_snapshot);
    REQUIRE(turn_calls == 1);
    REQUIRE(data.transaction_version == 1);
    REQUIRE_FALSE(find_scene(data, "root")->history.empty());
}

TEST_CASE("Story pending-turn guards reject unsafe calls",
          "[story][advance_player]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&,
                              const std::string&) {
        return response("Turn.");
    });

    REQUIRE_THROWS_AS(story.complete_turn(), std::runtime_error);

    REQUIRE(story.advance_player("One.").front().content == "Turn.");
    REQUIRE_THROWS_AS(story.advance_player("Two."), std::runtime_error);
    REQUIRE_THROWS_AS(story.weave_scene("root"), std::runtime_error);

    story.revert_active_turns(1);
    REQUIRE_THROWS_AS(story.complete_turn(), std::runtime_error);
    REQUIRE_NOTHROW(story.advance_player("Again."));
    REQUIRE_NOTHROW(story.complete_turn());

    const auto directory = temp_dir("rhapsode-pending-clear-");
    story.save(directory.string());
    REQUIRE_NOTHROW(story.advance_player("Pending."));
    story.load_save(directory.string());
    REQUIRE_THROWS_AS(story.complete_turn(), std::runtime_error);
    std::filesystem::remove_all(directory);
}

TEST_CASE("Story keeps player outputs separate from off-stage turns",
          "[story][turn_pipeline]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story,
        [](const std::string& id, const std::string&, const std::string&) {
            return response(id == "root" ? "Player turn." : "Away turn.");
        });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Advance the patrol") != nullptr);
    story.set_scheduler_callback([](const std::string&, const std::string&,
                                    const ReadToolCallback&) {
        return std::string{"away"};
    });
    const auto outputs = play_turn(story, "Act.");
    REQUIRE(outputs.size() == 1);
    REQUIRE(outputs.front().content == "Player turn.");
    REQUIRE(story.get_scene("away")->history.back().content == "Away turn.");
}

TEST_CASE("Story surfaces off-stage outputs when they merge into the active scene",
          "[story][turn_pipeline][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story,
        [](const std::string& id, const std::string&, const std::string&) {
            return response(id == "root" ? "Player turn."
                                         : "Scout steps from the treeline beside the player.");
        });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Reach the player") != nullptr);
    story.set_scheduler_callback([](const std::string&, const std::string&,
                                    const ReadToolCallback&) {
        return std::string{"away"};
    });
    story.set_lifecycle_callback(
        [](const std::string&, const std::string& user, const ReadToolCallback&) {
            const json context = json::parse(user.substr(user.find('{')));
            if (context["advanced_scene_id"] == "away") {
                return std::string{
                    R"({"ops":[{"op":"merge","from":"away","into":"root","reason":"reunion"}]})"};
            }
            return std::string{R"({"ops":[]})"};
        });

    const auto outputs = play_turn(story, "Act.");
    REQUIRE(outputs.size() == 2);
    REQUIRE(outputs[0].content == "Player turn.");
    REQUIRE(outputs[1].content ==
            "Scout steps from the treeline beside the player.");
    REQUIRE(story.get_scene("away") == nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") != nullptr);
}

TEST_CASE("Board lifecycle merges a fork after the main step without advancing it",
          "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    int away_narrations = 0;
    configure_story(story,
        [&](const std::string& id, const std::string&, const std::string&) {
            if (id == "away") ++away_narrations;
            return response(id == "root"
                ? "Scout stands beside you at the gate."
                : "Away should not run.");
        });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Reach the player") != nullptr);
    story.set_scheduler_callback([](const std::string&, const std::string&,
                                    const ReadToolCallback&) {
        return std::string{"away"};
    });
    story.set_lifecycle_callback(
        [](const std::string&, const std::string& user, const ReadToolCallback&) {
            const json context = json::parse(user.substr(user.find('{')));
            if (context["advanced_scene_id"] == "root") {
                return std::string{
                    R"({"ops":[{"op":"merge","from":"away","into":"root","reason":"co-presence"}]})"};
            }
            return std::string{R"({"ops":[]})"};
        });

    play_turn(story, "I meet the scout.");
    REQUIRE(away_narrations == 0);
    REQUIRE(story.get_scene("away") == nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") != nullptr);
    REQUIRE(story.scene_count() == 1);
}

TEST_CASE("Story applies lifecycle fork ops without a World queue", "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("The scout shoulders a pack.",
            R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Scout","line":"I will take the ridge.","action":"tightens the straps"}],"new_characters":[],"active_cast":["Scout"]})");
    });
    story.set_lifecycle_callback([](const std::string&, const std::string& user,
                                    const ReadToolCallback& read_tool) {
        REQUIRE(json::parse(read_tool("list_scenes", "{}"))[0]["scene_id"] ==
                "root");
        const json context = json::parse(user.substr(user.find('{')));
        REQUIRE(context["advanced_scene_id"] == "root");
        REQUIRE(context["dialogue"].size() == 1);
        REQUIRE(context["dialogue"][0]["speaker"] == "Scout");
        REQUIRE(context.contains("storylines"));
        return std::string{
            R"({"ops":[{"op":"fork","parent":"root","cast":["Scout"],"driving_intention":"Scout ridge"}]})"};
    });
    play_turn(story, "Go.");
    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.get_scene("root_f0_0") != nullptr);
    REQUIRE(story.world().find_in_scene("root_f0_0", "Scout") != nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") == nullptr);
}

TEST_CASE("Board lifecycle skips fork for a non-advanced parent",
          "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    world.enter_character("root", Character{"Guard", "Watchful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Quiet.");
    });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Patrol") != nullptr);
    story.set_lifecycle_callback(
        [](const std::string&, const std::string&, const ReadToolCallback&) {
            return std::string{
                R"({"ops":[{"op":"fork","parent":"away","cast":["Scout"],"driving_intention":"Deeper"}]})"};
        });
    play_turn(story, "Wait.");
    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.get_scene("away") != nullptr);
}

TEST_CASE("Board lifecycle skips stale merge targets and keeps earlier ops",
          "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    world.enter_character("root", Character{"Guard", "Watchful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Together at the gate.");
    });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Reach gate") != nullptr);
    REQUIRE(story.fork_scene(
        "root", "side", {"Guard"}, "Flank") != nullptr);
    story.set_lifecycle_callback(
        [](const std::string&, const std::string& user, const ReadToolCallback&) {
            const json context = json::parse(user.substr(user.find('{')));
            if (context["advanced_scene_id"] != "root")
                return std::string{R"({"ops":[]})"};
            return std::string{
                R"({"ops":[{"op":"merge","from":"away","into":"root","reason":"a"},)"
                R"({"op":"merge","from":"away","into":"side","reason":"stale"}]})"};
        });
    play_turn(story, "Meet them.");
    REQUIRE(story.get_scene("away") == nullptr);
    REQUIRE(story.get_scene("side") != nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") != nullptr);
}

TEST_CASE("Board lifecycle applies merge before fork in one response",
          "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    world.enter_character("root", Character{"Guard", "Watchful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Scout rejoins; Guard is sent ahead.");
    });
    REQUIRE(story.fork_scene(
        "root", "away", {"Scout"}, "Return") != nullptr);
    story.set_lifecycle_callback(
        [](const std::string&, const std::string& user, const ReadToolCallback&) {
            const json context = json::parse(user.substr(user.find('{')));
            if (context["advanced_scene_id"] != "root")
                return std::string{R"({"ops":[]})"};
            // Intentionally fork-first in JSON; engine sorts merge first.
            return std::string{
                R"({"ops":[{"op":"fork","parent":"root","cast":["Guard"],"driving_intention":"Scout ahead"},)"
                R"({"op":"merge","from":"away","into":"root","reason":"reunion"}]})"};
        });
    play_turn(story, "Split and reunite.");
    REQUIRE(story.get_scene("away") == nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") != nullptr);
    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.world().find_in_scene("root", "Guard") == nullptr);
}

TEST_CASE("Lifecycle policy rejects malformed board ops",
          "[story][lifecycle]") {
    TurnSummary turn;
    turn.scene_id = "root";
    auto decide = [&](const std::string& raw) {
        return request_lifecycle_decision(
            turn,
            [raw](const std::string&, const std::string&,
                  const ReadToolCallback&) { return raw; },
            [](const std::string&, const std::string&) {
                return std::string{"{}"};
            });
    };

    REQUIRE_FALSE(decide(
        R"({"ops":[{"op":"merge","from":"","into":"root"}]})"));
    REQUIRE_FALSE(decide(
        R"({"ops":[{"op":"fork","parent":"root","cast":["Scout"],"driving_intention":""}]})"));
    REQUIRE_FALSE(decide(
        R"({"ops":[{"op":"fork","parent":"root","cast":["Player"],"driving_intention":"Go"}]})"));
    REQUIRE(decide(R"({"ops":[]})"));
    auto ok = decide(
        R"({"ops":[{"op":"fork","parent":"root","cast":["Scout"],"driving_intention":"Go"}]})");
    REQUIRE(ok);
    REQUIRE(ok->ops.size() == 1);
    REQUIRE(ok->ops[0].kind == LifecycleOp::Kind::Fork);
}

TEST_CASE("Scheduler advances a starved off-stage scene within the cap",
          "[story][scheduler]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"A", "One", false});
    world.enter_character("root", Character{"B", "Two", false});
    world.enter_character("root", Character{"C", "Three", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story,
        [](const std::string& id, const std::string&, const std::string&) {
            return response(id + " advances.");
        });
    REQUIRE(story.fork_scene("root", "a", {"A"}, "A") != nullptr);
    REQUIRE(story.fork_scene("root", "b", {"B"}, "B") != nullptr);
    REQUIRE(story.fork_scene("root", "c", {"C"}, "C") != nullptr);
    // No LLM picks — starvation alone must fill the cap after enough turns.
    story.set_scheduler_callback([](const std::string&, const std::string&,
                                    const ReadToolCallback&) {
        return std::string{};
    });
    story.set_lifecycle_callback(
        [](const std::string&, const std::string&, const ReadToolCallback&) {
            return std::string{R"({"ops":[]})"};
        });

    for (int i = 0; i < 3; ++i)
        play_turn(story, "Idle.");
    REQUIRE(story.active_scene()->turn_index == 2);
    auto off_beats = [](const SceneData* scene) {
        return scene && scene->turn_index >= 0 ? scene->turn_index + 1 : 0;
    };
    const int off =
        off_beats(story.get_scene("a")) +
        off_beats(story.get_scene("b")) +
        off_beats(story.get_scene("c"));
    REQUIRE(off >= 2);
    REQUIRE(off <= 4);  // cap 2 per player turn across 2 starved turns max here
}

TEST_CASE("Story rejects an unknown active scene", "[story][invariant]") {
    Story story = Story::from_data(basic_scene());
    REQUIRE_THROWS_AS(story.set_active_scene("missing"), std::invalid_argument);
    REQUIRE(story.active_scene_id() == "root");
}

TEST_CASE("Story undo preserves the owned runtime", "[story][undo]") {
    Story story = Story::from_data(basic_scene());
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Moment advances.");
    });
    play_turn(story, "Forward.");
    REQUIRE(story.active_scene()->turn_index == 0);
    REQUIRE(story.revert_active_turns(1) == 1);
    REQUIRE(story.active_scene()->turn_index == -1);
    play_turn(story, "Again.");
    REQUIRE(story.active_scene()->turn_index == 0);
}

TEST_CASE("Story load reuses its configured runtime", "[story][persistence]") {
    Story story = Story::from_data(basic_scene());
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Moment advances.");
    });
    const auto directory = temp_dir("rhapsode-story-reuse-");
    story.save(directory.string());
    play_turn(story, "First.");
    story.load_save(directory.string());
    REQUIRE(story.active_scene()->turn_index == -1);
    play_turn(story, "Second.");
    REQUIRE(story.active_scene()->turn_index == 0);
    std::filesystem::remove_all(directory);
}

TEST_CASE("Story move assignment preserves its configured runtime",
          "[story][ownership]") {
    Story destination = Story::from_data(basic_scene("discarded"));
    configure_story(destination,
        [](const std::string&, const std::string&, const std::string&) {
            return response("Wrong runtime.");
        });

    World world;
    add_fact(world.graph(), "The gate is shut", "Gate", 0);
    Story source = Story::from_data(basic_scene(), std::move(world));
    bool read_source_world = false;
    source.set_llm_callback(
        [](const std::string&) { return std::string{"fallback"}; });
    source.set_narrator_llm_callback(
        [&](const std::string&, const std::string&, const std::string&,
            const ReadToolCallback& read_tool) {
            read_source_world =
                read_tool("query_graph", R"({"query":"gate"})")
                    .find("The gate is shut") != std::string::npos;
            return response("Moved runtime.");
        });

    destination = std::move(source);
    const auto outputs = play_turn(destination, "Open it.");

    REQUIRE(read_source_world);
    REQUIRE(outputs.size() == 1);
    REQUIRE(outputs.front().content == "Moved runtime.");
    REQUIRE(destination.active_scene_id() == "root");
    REQUIRE(destination.active_scene()->turn_index == 0);
}

TEST_CASE("Story owns explicit graph weaving", "[story][weaver][ownership]") {
    World world;
    const auto from = add_fact(world.graph(), "The gate is shut", "Gate", 0);
    const auto to = add_fact(world.graph(), "The key is nearby", "Key", 0);
    Story story = Story::from_data(basic_scene(), std::move(world));
    story.set_weaver_llm_callback([=](const std::string&) {
        return std::string{"{\"connect\":[{\"from\":"} +
            std::to_string(from) + ",\"to\":" + std::to_string(to) +
            R"(,"weight":0.7,"reason":"key"}],"disconnect":[],"reweight":[]})";
    });

    const WeaveResult result = story.weave_scene("root");

    REQUIRE(result.connected.size() == 1);
    REQUIRE(story.observations().all_edges().size() == 1);
    REQUIRE_THROWS_AS(story.weave_scene("missing"), std::invalid_argument);
}

TEST_CASE("Story save schema remains world, scene, and manifest blobs",
          "[story][persistence]") {
    World world;
    world.enter_character("root", Character{"Player", "The player", true});
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story,
        [](const std::string&, const std::string&, const std::string&) {
            return response("Wait.");
        });
    REQUIRE(story.fork_scene("root", "away", {"Scout"}, "Wait") != nullptr);
    const auto directory = temp_dir("rhapsode-story-schema-");
    story.save(directory.string());
    REQUIRE(std::filesystem::exists(directory / "world.json"));
    REQUIRE(std::filesystem::exists(directory / "story.json"));
    REQUIRE(std::filesystem::exists(directory / "root.json"));
    REQUIRE(std::filesystem::exists(directory / "away.json"));
    std::ifstream manifest_file(directory / "story.json");
    json manifest;
    manifest_file >> manifest;
    REQUIRE(manifest["scene_ids"] == json::array({"root", "away"}));
    manifest_file.close();
    std::filesystem::remove_all(directory);
}

TEST_CASE("Story timeline merges narrator and dialogue chronologically", "[story]") {
    SceneData scene = basic_scene();
    SceneMessage later;
    later.role = Role::Assistant;
    later.content = "later";
    later.timestamp = "2026-01-01T00:00:02Z";
    append_history_message(scene.history, later);
    SceneMessage earlier;
    earlier.role = Role::Assistant;
    earlier.content = "earlier";
    earlier.timestamp = "2026-01-01T00:00:01Z";
    append_history_message(scene.dialogue, earlier);
    Story story = Story::from_data(std::move(scene));
    const auto timeline = story.display_timeline("root");
    REQUIRE(timeline.size() == 2);
    REQUIRE(timeline.front().content == "earlier");
}

TEST_CASE("Attributed transcript orders a turn and returns exact speaker evidence",
          "[story][transcript]") {
    SceneData scene = basic_scene();
    const std::string timestamp = "2026-01-01T00:00:00Z";

    SceneMessage player;
    player.role = Role::User;
    player.content = "I approach.";
    player.timestamp = timestamp;
    player.metadata = {
        {"scene_kind", "player"}, {"turn", 4}, {"turn_ordinal", 0}};
    append_history_message(scene.history, player);

    SceneMessage narrator;
    narrator.role = Role::Assistant;
    narrator.content = "The guard raises a hand.";
    narrator.timestamp = timestamp;
    narrator.metadata = {
        {"scene_kind", "narrator"}, {"turn", 4}, {"turn_ordinal", 1}};
    append_history_message(scene.history, narrator);

    const std::string exact_line = "Stop. " + std::string(450, 'x');
    SceneMessage dialogue;
    dialogue.role = Role::Assistant;
    dialogue.content = exact_line;
    dialogue.timestamp = timestamp;
    dialogue.metadata = {
        {"scene_kind", "character"}, {"speaker", "Guard"},
        {"turn", 4}, {"turn_ordinal", 2}};
    append_history_message(scene.dialogue, dialogue);

    Story story = Story::from_data(std::move(scene));
    const auto timeline = story.display_timeline("root");
    REQUIRE(timeline.size() == 3);
    REQUIRE(timeline[0].content == "I approach.");
    REQUIRE(timeline[1].content == "The guard raises a hand.");
    REQUIRE(timeline[2].content == exact_line);

    const json result = json::parse(story.dispatch_tool(
        "root", "query_transcript", R"({"query":"Guard"})"));
    REQUIRE(result["spans"].size() == 2);
    REQUIRE(result["spans"][0]["speaker"] == "Guard");
    REQUIRE(result["spans"][0]["text"] == exact_line);
    REQUIRE(result["spans"][0]["turn"] == 4);
    REQUIRE(result["spans"][0]["ordinal"] == 2);
    REQUIRE(result["spans"][0]["legacy_order"] == false);
    REQUIRE(result["spans"][0]["retrieval_reason"] == "lexical");
}

TEST_CASE("Weaver batches independent groups in priority order",
          "[weaver][work_queue]") {
    WorldGraph graph;
    add_fact(graph, "A old", "A", 1);
    add_fact(graph, "A new", "A", 2);
    const auto old_id = add_fact(graph, "Gate closed", "Gate", 1);
    const auto new_id = add_fact(graph, "Gate open", "Gate", 5);
    Weaver weaver;
    std::vector<std::string> prompts;
    weaver.set_llm_callback([&](const std::string& prompt) {
        prompts.push_back(prompt);
        return std::string{
            R"({"results":[{"group_id":"g0","superseded":[{"id":)"} +
            std::to_string(old_id) + ",\"by\":" + std::to_string(new_id) +
            R"(}],"reason":"newer state"},{"group_id":"g1","superseded":[],"reason":"current"}]})";
    });
    weaver.rebuild_expiry_queue(graph, {"Gate"});
    const auto expired = weaver.drain_expiry_queue(graph, 9);
    REQUIRE(prompts.size() == 1);
    const auto gate = prompts.front().find("Gate closed");
    REQUIRE(prompts.front().find("Group g0") < gate);
    REQUIRE(gate < prompts.front().find("Group g1"));
    REQUIRE(expired.size() == 1);
    REQUIRE(graph.get_node(old_id)->valid_until == 5);
}

TEST_CASE("Weaver separates overlapping expiry groups",
          "[weaver][work_queue]") {
    WorldGraph graph;
    add_fact(graph, "A old", "A", 1);
    add_fact(graph, "B old", "B", 1);
    Node shared;
    shared.fact = "Shared current";
    shared.entities = {"A", "B"};
    shared.state = NodeState::Active;
    shared.created_at = 2;
    graph.add_node(std::move(shared));

    Weaver weaver;
    std::vector<std::string> prompts;
    weaver.set_llm_callback([&](const std::string& prompt) {
        prompts.push_back(prompt);
        return std::string{R"({"results":[]})"};
    });
    weaver.rebuild_expiry_queue(graph);
    REQUIRE(weaver.drain_expiry_queue(graph, 9).empty());
    REQUIRE(prompts.size() == 2);
    REQUIRE(prompts[0].find("Group g1") == std::string::npos);
    REQUIRE(prompts[1].find("Group g1") == std::string::npos);
}

TEST_CASE("Weaver rejects invalid cross-group supersession",
          "[weaver][work_queue]") {
    WorldGraph graph;
    const auto a_old = add_fact(graph, "A old", "A", 1);
    const auto a_new = add_fact(graph, "A new", "A", 4);
    const auto b_old = add_fact(graph, "B old", "B", 2);
    add_fact(graph, "B new", "B", 5);

    Weaver weaver;
    weaver.set_llm_callback([&](const std::string&) {
        return std::string{
            R"({"results":[{"group_id":"g0","superseded":[{"id":)"} +
            std::to_string(b_old) + ",\"by\":" + std::to_string(a_new) +
            R"(}],"reason":"cross-group"},{"group_id":"g1","superseded":[{"id":)" +
            std::to_string(a_new) + ",\"by\":" + std::to_string(a_old) +
            R"(}],"reason":"not newer"}]})";
    });
    weaver.rebuild_expiry_queue(graph, {"B"});
    REQUIRE(weaver.drain_expiry_queue(graph, 9).empty());
    REQUIRE(graph.get_node(a_new)->valid_until == -1);
    REQUIRE(graph.get_node(b_old)->valid_until == -1);
}

TEST_CASE("Weaver ignores malformed expiry batch results",
          "[weaver][work_queue]") {
    WorldGraph graph;
    add_fact(graph, "A old", "A", 1);
    add_fact(graph, "A new", "A", 2);
    const std::vector<std::string> responses = {
        "not JSON",
        R"({"results":{}})",
        R"({"results":[{"group_id":7,"superseded":[]}]})",
        R"({"results":[{"group_id":"g0","superseded":"invalid"}]})"};
    std::size_t response_index = 0;

    Weaver weaver;
    weaver.set_llm_callback([&](const std::string&) {
        return responses.at(response_index++);
    });
    for (std::size_t i = 0; i < responses.size(); ++i) {
        weaver.rebuild_expiry_queue(graph);
        REQUIRE(weaver.drain_expiry_queue(graph, 9).empty());
    }
}

TEST_CASE("Weaver stop leaves undrained groups queued", "[weaver][work_queue]") {
    using namespace std::chrono_literals;
    WorldGraph graph;
    for (int i = 0; i < 9; ++i) {
        const auto entity = "E" + std::to_string(i);
        add_fact(graph, entity + " old", entity, 1);
        add_fact(graph, entity + " new", entity, 2);
    }
    Weaver weaver;
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"results":[]})"};
    });
    weaver.rebuild_expiry_queue(graph);
    auto draining = std::async(std::launch::async, [&] {
        return weaver.drain_expiry_queue(graph, 8);
    });
    started.get_future().wait();
    weaver.stop_expiry_drain();
    REQUIRE(draining.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(draining.wait_for(2s) == std::future_status::ready);
    REQUIRE(draining.get().empty());
    REQUIRE_FALSE(weaver.expiry_queue_empty());
}
