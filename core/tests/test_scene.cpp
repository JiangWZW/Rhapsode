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
#include "rhapsode/history.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/node.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/turn_executor.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
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

void configure_executor(TurnExecutor& executor, TestNarratorCallback narrator) {
    executor.set_llm_callback([](const std::string&) { return std::string{"fallback"}; });
    executor.set_narrator_llm_callback(with_unused_read_tools(std::move(narrator)));
}

void configure_story(Story& story, TestNarratorCallback narrator) {
    story.set_llm_callback([](const std::string&) { return std::string{"fallback"}; });
    story.set_narrator_llm_callback(with_unused_read_tools(std::move(narrator)));
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

std::uint64_t prompt_hash(const std::string& text) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : text) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

}  // namespace

TEST_CASE("SceneData is a World-free aggregate", "[scene_data][ownership]") {
    STATIC_REQUIRE(std::is_aggregate_v<SceneData>);
    SceneData scene;
    scene.scene_id = "root";
    REQUIRE(scene.scene_id == "root");
    REQUIRE(scene.turn_index == 0);
}

TEST_CASE("Narrator instructions remain byte-identical across the refactor",
          "[narrator_prompt][characterization]") {
    const std::string instructions = build_narrator_instructions();
    REQUIRE(instructions.size() == 4016);
    REQUIRE(prompt_hash(instructions) == 0x15c41a86a90a0eedULL);
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

TEST_CASE("History append, snapshot, truncate, and round-trip", "[history]") {
    History history;
    for (int i = 0; i < 4; ++i) {
        SceneMessage message;
        message.role = i % 2 ? Role::Assistant : Role::User;
        message.content = std::to_string(i);
        history.append(std::move(message));
    }
    REQUIRE(history.size() == 4);
    REQUIRE(history.snapshot(2).front().content == "2");
    REQUIRE_FALSE(history.messages().front().timestamp.empty());
    history.truncate(3);
    REQUIRE(history.size() == 3);
    const json value = history;
    REQUIRE(value.get<History>().size() == 3);
}

TEST_CASE("History drops messages from a reverted turn", "[history]") {
    History history;
    for (int turn : {0, 1, 2}) {
        SceneMessage message;
        message.role = Role::Assistant;
        message.content = std::to_string(turn);
        message.metadata["turn"] = turn;
        history.append(std::move(message));
    }
    history.drop_from_turn(1);
    REQUIRE(history.size() == 1);
    REQUIRE(history.messages().front().content == "0");
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

TEST_CASE("New World memories accept call-scoped reflection configuration",
          "[world][character_memory]") {
    World world;
    int calls = 0;
    const LLMCallback reflection = [&](const std::string&) {
        ++calls;
        return std::string{"not json"};
    };
    world.enter_character("root", Character{"Scout", "Careful", false});
    Node perceived;
    perceived.fact = "The ridge is empty";
    perceived.entities = {"Ridge"};
    world.route_perceptions("root", {perceived}, 1);
    world.reflect_perceptions(2, reflection);
    REQUIRE(calls == 1);
}

TEST_CASE("Loaded World memories accept call-scoped reflection configuration",
          "[world][character_memory][persistence]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    int calls = 0;
    const LLMCallback reflection = [&](const std::string&) {
        ++calls;
        return std::string{"not json"};
    };
    world = World::from_json(world.to_json());
    Node perceived;
    perceived.fact = "The ridge is empty";
    perceived.entities = {"Ridge"};
    world.route_perceptions("root", {perceived}, 1);
    world.reflect_perceptions(2, reflection);
    REQUIRE(calls == 1);
}

TEST_CASE("Perceptions respect private and public audiences", "[world][memory]") {
    World world;
    world.enter_character("root", Character{"Alice", "A", false});
    world.enter_character("root", Character{"Bob", "B", false});
    Node private_fact;
    private_fact.fact = "Alice sees the key";
    private_fact.audience = {"Alice"};
    world.route_perceptions("root", {private_fact}, 1);
    Node public_fact;
    public_fact.fact = "The bell rings";
    world.route_perceptions("root", {public_fact}, 1);

    auto count = [&](const std::string& name) {
        int perceptions = 0;
        world.character_memories().at(name).beliefs().for_each(
            [&](const Node& node) {
                if (node.type == "perception") ++perceptions;
            }, false);
        return perceptions;
    };
    REQUIRE(count("Alice") == 2);
    REQUIRE(count("Bob") == 1);
}

TEST_CASE("TurnExecutor rejects graph-mismatched injected services",
          "[turn_executor][ownership]") {
    World world;
    WorldGraph other;
    Director wrong_director(other);
    Director director(world.graph());
    Weaver wrong_weaver(other);
    Weaver weaver(world.graph());
    REQUIRE_THROWS_AS(TurnExecutor(world, wrong_director, weaver),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(TurnExecutor(world, director, wrong_weaver),
                      std::invalid_argument);
}

TEST_CASE("Weaver activation preserves optional runtime semantics",
          "[weaver][ownership]") {
    WorldGraph graph;
    Weaver weaver(graph);
    REQUIRE_FALSE(weaver.active());
    weaver.set_interval(3);
    REQUIRE(weaver.active());
}

TEST_CASE("TurnExecutor returns associated generic turn effects",
          "[turn_executor][result][post_turn]") {
    World world;
    SceneData scene = basic_scene("root");
    const auto old_id = add_fact(world.graph(), "The gate is closed", "Gate", 1);
    const auto new_id = add_fact(world.graph(), "The gate is open", "Gate", 5);
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [](const std::string&, const std::string&, const std::string&) {
        return response("The gate groans.",
            R"({"transitions":[],"new_nodes":[{"fact":"Wind crosses the gate","entities":["Gate"]}],"speech_turns":[],"new_characters":[],"active_cast":[]})");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback([&](const std::string&) {
        return std::string{"{\"connect\":[{\"from\":"} +
            std::to_string(old_id) + ",\"to\":" + std::to_string(new_id) +
            R"(,"weight":0.8,"reason":"state"}],"disconnect":[],"reweight":[]})";
    });
    weaver.set_local_llm_callback([&](const std::string&) {
        return std::string{"{\"superseded\":[{\"id\":"} +
            std::to_string(old_id) + ",\"by\":" + std::to_string(new_id) +
            R"(}],"reason":"newer state"})";
    });

    const TurnResult result = executor.run_player_turn(scene, "Look.");
    REQUIRE(result.scene_id == "root");
    REQUIRE(result.completed_turn == 1);
    REQUIRE(result.effects.created_nodes.size() == 1);
    REQUIRE(result.effects.expired_nodes.size() == 1);
    REQUIRE(result.effects.expired_nodes.front().id == old_id);
    const auto edges = world.graph().all_edges();
    REQUIRE(std::any_of(edges.begin(), edges.end(), [&](const EdgeInfo& edge) {
        return edge.from_id == old_id && edge.to_id == new_id &&
               edge.data.active && edge.data.weight == 0.8f;
    }));
}

TEST_CASE("TurnExecutor keeps narrator and dialogue histories separate", "[turn_executor]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Guard", "Alert", false});
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [](const std::string&, const std::string&, const std::string&) {
        return response("The guard raises a hand.",
            R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Guard","line":"Stop.","action":"blocks the door"}],"new_characters":[],"active_cast":["Guard"]})");
    });
    const auto result = executor.run_player_turn(scene, "Approach.");
    REQUIRE(result.outputs.size() == 2);
    REQUIRE(scene.history.size() == 2);
    REQUIRE(scene.dialogue.size() == 1);
    REQUIRE(scene.dialogue.messages().front().content == "Stop. (blocks the door)");
}

TEST_CASE("TurnExecutor active_cast adds presence without ejecting cast", "[turn_executor]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Alice", "A", false});
    world.enter_character("elsewhere", Character{"Bob", "B", false});
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [](const std::string&, const std::string&, const std::string&) {
        return response("Bob arrives.",
            R"({"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Bob"]})");
    });
    executor.run_player_turn(scene, "Wait.");
    REQUIRE(world.find_in_scene("root", "Alice") != nullptr);
    REQUIRE(world.find_in_scene("root", "Bob") != nullptr);
}

TEST_CASE("TurnExecutor retries invalid Player speech without leaking state",
          "[turn_executor][transaction]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Guard", "Alert", false});
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    int calls = 0;
    configure_executor(executor, [&](const std::string&, const std::string&, const std::string&) {
        ++calls;
        if (calls == 1)
            return response("Wrong.",
                R"({"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Player","line":"No"}],"new_characters":[],"active_cast":["Guard"]})");
        return response("Corrected.");
    });
    const auto result = executor.run_player_turn(scene, "Act.");
    REQUIRE(calls == 2);
    REQUIRE(result.outputs.front().content == "Corrected.");
    REQUIRE(scene.turn_index == 1);
}

TEST_CASE("TurnExecutor rolls back failed turns and can be reused",
          "[turn_executor][transaction]") {
    World world;
    SceneData scene = basic_scene();
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor,
        [](const std::string&, const std::string&, const std::string&) -> std::string {
            throw std::runtime_error("narrator unavailable");
        });
    REQUIRE_THROWS_AS(executor.run_player_turn(scene, "Act."), std::runtime_error);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 0);
    REQUIRE(world.graph().size() == 0);

    executor.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&,
           const ReadToolCallback&) {
            return response("Recovered.");
        });
    REQUIRE(executor.run_player_turn(scene, "Again.").outputs.front().content == "Recovered.");
}

TEST_CASE("TurnExecutor autonomous turns remain associated with their SceneData",
          "[turn_executor][result]") {
    World world;
    SceneData first = basic_scene("first");
    SceneData second = basic_scene("second");
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor,
        [](const std::string& id, const std::string&, const std::string&) {
            return response("Narration for " + id + ".");
        });
    REQUIRE(executor.run_player_turn(first, "Act.").scene_id == "first");
    const auto result = executor.run_autonomous_turn(second, "Continue.");
    REQUIRE(result.scene_id == "second");
    REQUIRE(result.outputs.front().content == "Narration for second.");
}

TEST_CASE("TurnExecutor completes post-turn work before returning",
          "[turn_executor][post_turn]") {
    using namespace std::chrono_literals;
    World world;
    SceneData scene = basic_scene();
    add_fact(world.graph(), "Gate shut", "Gate", 1);
    add_fact(world.graph(), "Torch lit", "Torch", 1);
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [](const std::string&, const std::string&, const std::string&) {
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

    auto running = std::async(std::launch::async, [&] {
        return executor.run_player_turn(scene, "Wait.");
    });
    started.get_future().wait();
    REQUIRE(running.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(running.wait_for(2s) == std::future_status::ready);
    REQUIRE(running.get().completed_turn == 1);
}

TEST_CASE("TurnExecutor preserves post-turn callback order",
          "[turn_executor][post_turn][characterization]") {
    World world;
    SceneData scene = basic_scene();
    world.enter_character("root", Character{"Scout", "Careful", false});
    add_fact(world.graph(), "The gate is closed", "Gate", 1);
    add_fact(world.graph(), "The gate is open", "Gate", 2);
    for (int i = 0; i < 7; ++i) {
        SceneMessage prior;
        prior.role = i % 2 ? Role::Assistant : Role::User;
        prior.content = "Prior " + std::to_string(i);
        scene.history.append(std::move(prior));
    }

    std::vector<std::string> events;
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [&](const std::string&, const std::string&,
                             const std::string&) {
        events.push_back("narrator");
        return response("Wind crosses the gate.",
            R"({"transitions":[],"new_nodes":[{"fact":"Wind crosses the gate","entities":["Gate"]}],"speech_turns":[],"new_characters":[],"active_cast":["Scout"]})");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback([&](const std::string&) {
        events.push_back("weave");
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });
    weaver.set_local_llm_callback([&](const std::string&) {
        events.push_back("expiry");
        return std::string{R"({"superseded":[],"reason":"current"})"};
    });
    executor.set_reflection_llm_callback([&](const std::string&) {
        events.push_back("reflection");
        return std::string{R"({"thoughts":[]})"};
    });
    executor.set_downsampler_callback([&](const std::string&) {
        events.push_back("downsample");
        return std::string{"summary"};
    });

    REQUIRE(executor.run_player_turn(scene, "Look.").completed_turn == 1);
    REQUIRE(events == std::vector<std::string>{
        "narrator", "weave", "expiry", "reflection", "downsample"});
}

TEST_CASE("TurnExecutor post-turn failures remain non-fatal",
          "[turn_executor][post_turn][characterization]") {
    World world;
    SceneData scene = basic_scene();
    add_fact(world.graph(), "Gate shut", "Gate", 1);
    add_fact(world.graph(), "Torch lit", "Torch", 1);
    Director director(world.graph());
    Weaver weaver(world.graph());
    TurnExecutor executor(world, director, weaver);
    configure_executor(executor, [](const std::string&, const std::string&,
                            const std::string&) {
        return response("Time passes.");
    });
    weaver.set_interval(1);
    weaver.set_llm_callback(
        [](const std::string&) -> std::string {
            throw std::runtime_error("weaver unavailable");
        });

    const TurnResult result = executor.run_player_turn(scene, "Wait.");
    REQUIRE(result.completed_turn == 1);
    REQUIRE(result.outputs.front().content == "Time passes.");
    REQUIRE(scene.turn_index == 1);
}

TEST_CASE("Story keeps player outputs separate from off-stage turns",
          "[story][turn_executor]") {
    Story story = Story::from_data(basic_scene());
    story.fork_scene("root", "away", {}, "Advance the patrol");
    configure_story(story,
        [](const std::string& id, const std::string&, const std::string&) {
            return response(id == "root" ? "Player beat." : "Away beat.");
        });
    story.set_scheduler_callback([](const std::string&, const std::string&,
                                    const ReadToolCallback&) {
        return std::string{"away"};
    });
    const auto outputs = story.advance_scene("Act.");
    REQUIRE(outputs.size() == 1);
    REQUIRE(outputs.front().content == "Player beat.");
    REQUIRE(story.get_scene("away")->history.messages().back().content == "Away beat.");
}

TEST_CASE("Story applies lifecycle verdicts without a World queue", "[story][lifecycle]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    Story story = Story::from_data(basic_scene(), std::move(world));
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("The scout leaves.");
    });
    story.set_lifecycle_callback([](const std::string&, const std::string&) {
        return std::string{R"({"fork":{"cast":["Scout"],"driving_intention":"Scout ridge"},"merge_into":null,"conclude":null,"exited":[]})"};
    });
    story.advance_scene("Go.");
    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.get_scene("root_f0_0") != nullptr);
    REQUIRE(story.world().find_in_scene("root_f0_0", "Scout") != nullptr);
    REQUIRE(story.world().find_in_scene("root", "Scout") == nullptr);
}

TEST_CASE("Story undo preserves the owned runtime", "[story][undo]") {
    Story story = Story::from_data(basic_scene());
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Moment advances.");
    });
    story.advance_scene("Forward.");
    REQUIRE(story.active_scene()->turn_index == 1);
    REQUIRE(story.revert_active_turns(1) == 1);
    REQUIRE(story.active_scene()->turn_index == 0);
    story.advance_scene("Again.");
    REQUIRE(story.active_scene()->turn_index == 1);
}

TEST_CASE("Story load reuses its configured runtime", "[story][persistence]") {
    Story story = Story::from_data(basic_scene());
    configure_story(story, [](const std::string&, const std::string&, const std::string&) {
        return response("Moment advances.");
    });
    const auto directory = temp_dir("rhapsode-story-reuse-");
    story.save(directory.string());
    story.advance_scene("First.");
    story.load_save(directory.string());
    REQUIRE(story.active_scene()->turn_index == 0);
    story.advance_scene("Second.");
    REQUIRE(story.active_scene()->turn_index == 1);
    std::filesystem::remove_all(directory);
}

TEST_CASE("Story save schema remains world, scene, and manifest blobs",
          "[story][persistence]") {
    Story story = Story::from_data(basic_scene());
    story.fork_scene("root", "away", {}, "Wait");
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
    scene.history.append(later);
    SceneMessage earlier;
    earlier.role = Role::Assistant;
    earlier.content = "earlier";
    earlier.timestamp = "2026-01-01T00:00:01Z";
    scene.dialogue.append(earlier);
    Story story = Story::from_data(std::move(scene));
    const auto timeline = story.display_timeline("root");
    REQUIRE(timeline.size() == 2);
    REQUIRE(timeline.front().content == "earlier");
}

TEST_CASE("Weaver queue prioritizes groups and applies supersession",
          "[weaver][work_queue]") {
    WorldGraph graph;
    add_fact(graph, "A old", "A", 1);
    add_fact(graph, "A new", "A", 2);
    const auto old_id = add_fact(graph, "Gate closed", "Gate", 1);
    const auto new_id = add_fact(graph, "Gate open", "Gate", 5);
    Weaver weaver(graph);
    std::vector<std::string> prompts;
    weaver.set_local_llm_callback([&](const std::string& prompt) {
        prompts.push_back(prompt);
        if (prompt.find("Gate closed") != std::string::npos)
            return std::string{"{\"superseded\":[{\"id\":"} +
                std::to_string(old_id) + ",\"by\":" + std::to_string(new_id) +
                R"(}],"reason":"newer state"})";
        return std::string{R"({"superseded":[],"reason":"current"})"};
    });
    weaver.rebuild_expiry_queue({"Gate"});
    const auto expired = weaver.drain_expiry_queue(9);
    REQUIRE(prompts.size() == 2);
    REQUIRE(prompts.front().find("Gate closed") != std::string::npos);
    REQUIRE(expired.size() == 1);
    REQUIRE(graph.get_node(old_id)->valid_until == 5);
}

TEST_CASE("Weaver stop leaves undrained groups queued", "[weaver][work_queue]") {
    using namespace std::chrono_literals;
    WorldGraph graph;
    add_fact(graph, "A old", "A", 1);
    add_fact(graph, "A new", "A", 2);
    add_fact(graph, "B old", "B", 1);
    add_fact(graph, "B new", "B", 2);
    Weaver weaver(graph);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_local_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"superseded":[],"reason":"current"})"};
    });
    weaver.rebuild_expiry_queue();
    auto draining = std::async(std::launch::async, [&] {
        return weaver.drain_expiry_queue(8);
    });
    started.get_future().wait();
    weaver.stop_expiry_drain();
    REQUIRE(draining.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(draining.wait_for(2s) == std::future_status::ready);
    REQUIRE(draining.get().empty());
    REQUIRE_FALSE(weaver.expiry_queue_empty());
}
