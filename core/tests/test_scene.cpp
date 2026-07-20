#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include "rhapsode/scene_message.h"
#include "rhapsode/history.h"
#include "rhapsode/character.h"
#include "rhapsode/scene.h"
#include "rhapsode/director.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/story.h"
#include "rhapsode/weaver.h"

using namespace rhapsode;
using json = nlohmann::json;

TEST_CASE("SceneMessage JSON round-trip", "[scene_message]") {
    SceneMessage msg;
    msg.role = Role::User;
    msg.content = "Hello world";
    msg.timestamp = "2026-05-05T14:00:00Z";
    msg.metadata = json{{"key", "value"}};

    json j;
    to_json(j, msg);

    REQUIRE(j["role"] == "user");
    REQUIRE(j["content"] == "Hello world");
    REQUIRE(j["timestamp"] == "2026-05-05T14:00:00Z");
    REQUIRE(j["metadata"]["key"] == "value");

    SceneMessage restored;
    from_json(j, restored);

    REQUIRE(restored.role == Role::User);
    REQUIRE(restored.content == "Hello world");
    REQUIRE(restored.timestamp == "2026-05-05T14:00:00Z");
    REQUIRE(restored.metadata["key"] == "value");
}

TEST_CASE("SceneMessage roles serialize correctly", "[scene_message]") {
    for (auto role : {Role::System, Role::User, Role::Assistant}) {
        SceneMessage msg;
        msg.role = role;
        msg.content = "test";
        json j;
        to_json(j, msg);
        SceneMessage restored;
        from_json(j, restored);
        REQUIRE(restored.role == role);
    }
}

TEST_CASE("History append and snapshot", "[history]") {
    History h;
    REQUIRE(h.size() == 0);

    SceneMessage m1;
    m1.role = Role::User;
    m1.content = "first";
    h.append(m1);

    SceneMessage m2;
    m2.role = Role::Assistant;
    m2.content = "second";
    h.append(m2);

    REQUIRE(h.size() == 2);
    REQUIRE(h.messages()[0].content == "first");
    REQUIRE(h.messages()[1].content == "second");

    SECTION("snapshot returns all when n is nullopt") {
        auto snap = h.snapshot();
        REQUIRE(snap.size() == 2);
    }

    SECTION("snapshot returns last n") {
        auto snap = h.snapshot(1);
        REQUIRE(snap.size() == 1);
        REQUIRE(snap[0].content == "second");
    }

    SECTION("snapshot returns all when n > size") {
        auto snap = h.snapshot(100);
        REQUIRE(snap.size() == 2);
    }

    SECTION("clear empties history") {
        h.clear();
        REQUIRE(h.size() == 0);
    }
}

TEST_CASE("History drop_from_turn", "[history]") {
    History h;
    SceneMessage m1;
    m1.role = Role::Assistant;
    m1.content = "turn 0";
    m1.metadata = {{"turn", 0}, {"scene_kind", "character"}};
    h.append(m1);

    SceneMessage m2;
    m2.role = Role::Assistant;
    m2.content = "turn 1";
    m2.metadata = {{"turn", 1}, {"scene_kind", "character"}};
    h.append(m2);

    h.drop_from_turn(1);
    REQUIRE(h.size() == 1);
    REQUIRE(h.messages()[0].content == "turn 0");
}

TEST_CASE("History sets timestamp on append", "[history]") {
    History h;
    SceneMessage msg;
    msg.role = Role::User;
    msg.content = "test";

    h.append(msg);
    REQUIRE_FALSE(h.messages()[0].timestamp.empty());
}

TEST_CASE("History JSON round-trip", "[history]") {
    History h;
    SceneMessage m1;
    m1.role = Role::User;
    m1.content = "hello";
    m1.timestamp = "2026-01-01T00:00:00Z";
    h.append(m1);

    json j;
    to_json(j, h);
    REQUIRE(j.is_array());
    REQUIRE(j.size() == 1);

    History restored;
    from_json(j, restored);
    REQUIRE(restored.size() == 1);
    REQUIRE(restored.messages()[0].content == "hello");
}

TEST_CASE("Character JSON round-trip", "[character]") {
    Character c{"Barkeep", "A gruff dwarf", false};

    json j;
    to_json(j, c);
    REQUIRE(j["name"] == "Barkeep");
    REQUIRE(j["description"] == "A gruff dwarf");
    REQUIRE(j["is_player"] == false);

    Character restored;
    from_json(j, restored);
    REQUIRE(restored.name == "Barkeep");
    REQUIRE(restored.description == "A gruff dwarf");
    REQUIRE(restored.is_player == false);
}

TEST_CASE("Character is_player defaults to false", "[character]") {
    json j = {{"name", "NPC"}, {"description", "test"}};
    Character c;
    from_json(j, c);
    REQUIRE(c.is_player == false);
}

TEST_CASE("Scene JSON round-trip", "[scene]") {
    Scene scene;
    scene.title = "Test Scene";
    scene.system_prompt = "You are a narrator.";
    scene.world().characters.push_back({"Player", "An adventurer", true});
    scene.world().characters.push_back({"NPC", "A villager", false});

    SceneMessage seed;
    seed.role = Role::Assistant;
    seed.content = "Welcome!";
    seed.timestamp = "2026-01-01T00:00:00Z";
    scene.history.append(seed);

    json j = scene.to_json();
    REQUIRE(j["title"] == "Test Scene");
    REQUIRE(j["characters"].size() == 2);
    REQUIRE(j["history"].size() == 1);

    Scene restored = Scene::from_json(j);
    REQUIRE(restored.title == "Test Scene");
    REQUIRE(restored.system_prompt == "You are a narrator.");
    REQUIRE(restored.world().characters.size() == 2);
    REQUIRE(restored.world().characters[0].is_player == true);
    REQUIRE(restored.history.size() == 1);
    REQUIRE(restored.history.messages()[0].content == "Welcome!");
}

TEST_CASE("Scene loads seed_messages from scenario JSON", "[scene]") {
    json j = {
        {"title", "Tavern"},
        {"system_prompt", "Narrate."},
        {"characters", json::array()},
        {"seed_messages", {{{"role", "assistant"}, {"content", "You enter..."}}}}
    };

    Scene scene = Scene::from_json(j);
    REQUIRE(scene.history.size() == 1);
    REQUIRE(scene.history.messages()[0].role == Role::Assistant);
    REQUIRE(scene.history.messages()[0].content == "You enter...");
}

TEST_CASE("WorldGraph DOT renders escaped nodes, states, and edge activity", "[world_graph]") {
    WorldGraph graph;

    Node active;
    active.fact = "The \"gate\" opens.\nGuards react.";
    active.state = NodeState::Active;
    active.created_at = 1;
    const auto active_id = graph.add_node(std::move(active)).id;

    Node foreshadowed;
    foreshadowed.fact = "A shadow waits";
    foreshadowed.state = NodeState::Foreshadowed;
    foreshadowed.created_at = 2;
    const auto foreshadowed_id = graph.add_node(std::move(foreshadowed)).id;

    REQUIRE(graph.add_relation(active_id, foreshadowed_id));
    REQUIRE(graph.set_edge_active(active_id, foreshadowed_id, false));

    const std::string dot = graph.to_dot();
    REQUIRE(dot.find("digraph WorldGraph") != std::string::npos);
    REQUIRE(dot.find("The \\\"gate\\\" opens.\\nGuards react.") != std::string::npos);
    REQUIRE(dot.find("fillcolor=\"#a6e3a1\"") != std::string::npos);
    REQUIRE(dot.find("fillcolor=\"#f9e2af\"") != std::string::npos);
    REQUIRE(dot.find("n1 -> n2 [color=\"#a6adc8\", style=dashed]") != std::string::npos);
}

TEST_CASE("WorldGraph serialization preserves edge metadata and legacy inputs", "[world_graph]") {
    const json serialized = {
        {"next_id", 3},
        {"nodes", json::array({
            {{"id", 1}, {"fact", "First"}, {"created_at", 1}},
            {{"id", 2}, {"fact", "Second"}, {"created_at", 2}},
        })},
        {"edges", json::array({
            {{"from", 1}, {"to", 2}, {"confidence", 0.4f},
             {"created_at", 7}, {"active", false}, {"kind", "evidence"}},
        })},
    };

    WorldGraph restored = WorldGraph::from_json(serialized);
    REQUIRE(restored.size() == 2);
    REQUIRE(restored.all_edges().size() == 1);
    const EdgeInfo edge = restored.all_edges().front();
    REQUIRE(edge.from_id == 1);
    REQUIRE(edge.to_id == 2);
    REQUIRE(edge.data.weight == 0.4f);
    REQUIRE(edge.data.created_at == 7);
    REQUIRE_FALSE(edge.data.active);
    REQUIRE(edge.data.kind == "evidence");
    REQUIRE(restored.to_json()["edges"][0]["weight"] == 0.4f);

    WorldGraph legacy = WorldGraph::from_legacy_node_pool_json({
        {"next_id", 9},
        {"nodes", serialized["nodes"]},
    });
    Node next;
    next.fact = "Legacy next";
    REQUIRE(legacy.add_node(std::move(next)).id == 9);
    REQUIRE(legacy.all_edges().empty());
}

TEST_CASE("SceneLoop state transitions", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    SceneLoop loop;
    REQUIRE(loop.state() == LoopState::Idle);

    loop.load_scene(scene);
    REQUIRE(loop.state() == LoopState::WaitingForInput);

    std::string captured_instructions;
    std::string captured_turn_state;
    SceneMessage captured_result;

    Director director(scene.world().world_graph);
    loop.set_director(&director);

    loop.set_narrator_llm_callback([&](const std::string& /*scene_id*/,
                                       const std::string& instructions,
                                       const std::string& turn_state) {
        captured_instructions = instructions;
        captured_turn_state = turn_state;
        REQUIRE(instructions.find("<<<RHAPSODE_JSON>>>") != std::string::npos);
        REQUIRE(turn_state.find("### Turn transcript") != std::string::npos);
        return "The tavern is warm.";
    });

    loop.set_llm_callback([&](const std::string& prompt) {
        (void)prompt;
        return "fallback";
    });

    loop.set_turn_complete_callback([&](const SceneMessage& msg) {
        captured_result = msg;
    });

    loop.submit_input("I enter the tavern.");

    REQUIRE(loop.state() == LoopState::WaitingForInput);
    REQUIRE(scene.history.size() == 2);
    REQUIRE(scene.history.messages()[0].role == Role::User);
    REQUIRE(scene.history.messages()[0].content == "I enter the tavern.");
    REQUIRE(scene.history.messages()[1].role == Role::Assistant);
    REQUIRE(scene.history.messages()[1].content == "The tavern is warm.");
    REQUIRE(captured_instructions.find("<<<RHAPSODE_JSON>>>") != std::string::npos);
    REQUIRE(captured_turn_state.find("I enter the tavern.") != std::string::npos);
    REQUIRE(captured_result.content == "The tavern is warm.");
}

TEST_CASE("SceneLoop parses sentinel and fallback narrator plans", "[scene_loop]") {
    auto run_response = [](Scene& scene, std::string response) {
        scene.scene_id = "root";
        scene.title = "Root";
        scene.system_prompt = "Narrate.";
        Director director(scene.world().world_graph);
        SceneLoop loop;
        loop.set_director(&director);
        loop.set_llm_callback([](const std::string&) { return "fallback"; });
        loop.set_narrator_llm_callback(
            [response = std::move(response)](
                const std::string&, const std::string&, const std::string&) {
                return response;
            });
        return loop.run_player_turn(scene, "Continue.");
    };

    SECTION("sentinel split") {
        Scene scene;
        const auto result = run_response(scene, R"(You step forward.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})");

        REQUIRE(result.outputs.size() == 1);
        REQUIRE(result.outputs[0].content == "You step forward.");
    }

    SECTION("fallback takes the outermost plan object") {
        Scene scene;
        scene.enter_character(Character{"Guard", "A guard", false});
        const auto result = run_response(
            scene,
            R"(A stray { brace } appears before the actual plan.
{"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Guard","line":"Hold."}],"new_characters":[],"active_cast":["Guard"]})");

        REQUIRE(result.outputs.size() == 2);
        REQUIRE(result.outputs[0].content == "A stray { brace } appears before the actual plan.");
        REQUIRE(result.outputs[1].content == "Hold.");
    }

    SECTION("normalizes smart quotes before parsing") {
        const std::string lq = "\xE2\x80\x9C";
        const std::string rq = "\xE2\x80\x9D";
        const std::string raw =
            "Narration.\n<<<RHAPSODE_JSON>>>\n{" +
            lq + "transitions" + rq + ":[]," +
            lq + "new_nodes" + rq + ":[{" +
            lq + "fact" + rq + ":" + lq + "A bell rings" + rq + "," +
            lq + "entities" + rq + ":[]}]," +
            lq + "speech_turns" + rq + ":[]," +
            lq + "new_characters" + rq + ":[]," +
            lq + "active_cast" + rq + ":[]}";
        Scene scene;
        const auto result = run_response(scene, raw);

        REQUIRE(result.outputs[0].content == "Narration.");
        REQUIRE(result.director.new_nodes.size() == 1);
        REQUIRE(result.director.new_nodes[0].fact == "A bell rings");
    }
}

TEST_CASE("active_cast is presence-only: brings NPCs on-stage, never ejects", "[scene_loop]") {
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Root";
    scene.system_prompt = "Narrate.";
    Character alice{"Alice", "A guard", false};
    Character bob{"Bob", "A scout", false};
    scene.enter_character(std::move(alice));
    scene.enter_character(std::move(bob));

    Director director(scene.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });

    // Bob omitted from active_cast is NOT ejected -- removing a character from a
    // storyline is the lifecycle verdict's job, not active_cast's.
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Narration.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Alice"]})"};
        });
    loop.run_player_turn(scene, "Continue.");
    REQUIRE(scene.find_on_stage("Alice") != nullptr);
    REQUIRE(scene.find_on_stage("Bob") != nullptr);

    // A roster character currently off this scene is brought on when named.
    scene.exit_character("Bob");
    REQUIRE(scene.find_on_stage("Bob") == nullptr);
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Narration.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Alice","Bob"]})"};
        });
    loop.run_player_turn(scene, "Continue.");
    REQUIRE(scene.find_on_stage("Bob") != nullptr);
}

TEST_CASE("active_cast does not resolve arbitrary substrings", "[scene_loop]") {
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Root";
    scene.system_prompt = "Narrate.";
    Character alice{"Alice", "A guard", false};
    Character albert{"Albert", "A scout", false};
    scene.enter_character(std::move(alice));
    scene.enter_character(std::move(albert));
    scene.exit_character("Alice");
    scene.exit_character("Albert");

    Director director(scene.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Narration.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Al"]})"};
        });
    loop.run_player_turn(scene, "Continue.");
    REQUIRE(scene.find_on_stage("Alice") == nullptr);
    REQUIRE(scene.find_on_stage("Albert") == nullptr);

    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Narration.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Alice"]})"};
        });
    loop.run_player_turn(scene, "Continue.");
    REQUIRE(scene.find_on_stage("Alice") != nullptr);
    REQUIRE(scene.find_on_stage("Albert") == nullptr);
}

TEST_CASE("route_perception respects audience and public beats", "[scene_loop]") {
    Scene scene;
    Character alice{"Alice", "A guard", false};
    Character bob{"Bob", "A scout", false};
    scene.enter_character(std::move(alice));
    scene.enter_character(std::move(bob));

    Node private_node;
    private_node.fact = "Alice notices the hidden switch";
    private_node.entities = {"Switch"};
    private_node.audience = {"Alice"};

    scene.world().route_perceptions(scene.scene_id, {private_node}, 2);

    int alice_perceptions = 0;
    scene.world().character_memories.at("Alice").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++alice_perceptions;
    }, false);
    int bob_perceptions = 0;
    scene.world().character_memories.at("Bob").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++bob_perceptions;
    }, false);

    REQUIRE(alice_perceptions == 1);
    REQUIRE(bob_perceptions == 0);

    Node public_node;
    public_node.fact = "The gate opens";
    public_node.entities = {"Gate"};

    scene.world().route_perceptions(scene.scene_id, {public_node}, 3);

    alice_perceptions = 0;
    scene.world().character_memories.at("Alice").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++alice_perceptions;
    }, false);
    bob_perceptions = 0;
    scene.world().character_memories.at("Bob").beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++bob_perceptions;
    }, false);

    REQUIRE(alice_perceptions == 2);
    REQUIRE(bob_perceptions == 1);
}

TEST_CASE("World death mutation clears every scene membership", "[world]") {
    World world;
    Character scout{"Scout", "A careful scout", false};
    scout.join_scene("root");
    scout.join_scene("ridge");
    world.characters.push_back(std::move(scout));

    REQUIRE(world.mark_character_dead("Scout"));
    const Character* marked = world.find_character("Scout");
    REQUIRE(marked != nullptr);
    REQUIRE(marked->dead);
    REQUIRE(marked->scene_ids.empty());
    REQUIRE_FALSE(world.mark_character_dead("Unknown"));
}

TEST_CASE("World reapplies reflection configuration after loading memories",
          "[world][character_memory][persistence]") {
    World world;
    world.characters.push_back(Character{"Scout", "A careful scout", false});
    world.character_memories.emplace("Scout", CharacterMemory{"Scout"});

    int reflection_calls = 0;
    world.set_reflection_llm_callback([&](const std::string&) {
        ++reflection_calls;
        return std::string{"not json"};
    });

    const auto save_dir = std::filesystem::temp_directory_path() /
        ("rhapsode-world-reflection-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    world.save(save_dir.string());
    world.load_save(save_dir.string());

    world.character_memories.at("Scout").route_fact(
        "The ridge is empty", {"Ridge"}, 1);
    world.reflect_perceptions(2);

    REQUIRE(reflection_calls == 1);
    std::filesystem::remove_all(save_dir);
}

TEST_CASE("CharacterMemory reflection preserves its current graph contract",
          "[character_memory][reflection]") {
    auto find_fact = [](const std::vector<Node>& nodes,
                        const std::string& fact) -> const Node* {
        for (const auto& node : nodes)
            if (node.fact == fact) return &node;
        return nullptr;
    };

    SECTION("no callback leaves perceptions live") {
        CharacterMemory memory("Maren");
        memory.route_fact("Ash hid the key", {"Ash"}, 5);

        memory.reflect_perceptions(6, "A wary guard");

        const auto nodes = memory.beliefs().all_nodes(true);
        const Node* perception = find_fact(nodes, "Ash hid the key");
        REQUIRE(perception != nullptr);
        REQUIRE(perception->type == "perception");
        REQUIRE(perception->valid_until == -1);
    }

    SECTION("successful response preserves prompt, parsing, edges, and weights") {
        CharacterMemory memory("Maren");
        memory.seed_belief("I trust Ash", {"Ash"}, 0, 4.0f);
        memory.seed_belief("The gate is safe", {"Gate"}, 0, 4.0f);
        memory.route_fact("Ash hid the key", {"Ash"}, 5);

        std::string captured_prompt;
        memory.set_reflection_llm_callback([&](const std::string& prompt) {
            captured_prompt = prompt;
            return std::string{R"({"thoughts":[{"id":0,"thought":"My belief: I cannot trust Ash now","weight":12,"relation":"contradicts"}]})"};
        });

        memory.reflect_perceptions(6, "A wary guard");

        REQUIRE(captured_prompt.find("You are Maren.") != std::string::npos);
        REQUIRE(captured_prompt.find("Who I am: A wary guard") != std::string::npos);
        REQUIRE(captured_prompt.find("I trust Ash") != std::string::npos);
        REQUIRE(captured_prompt.find("Ash hid the key") != std::string::npos);

        const auto nodes = memory.beliefs().all_nodes(true);
        const Node* prior = find_fact(nodes, "I trust Ash");
        const Node* unrelated = find_fact(nodes, "The gate is safe");
        const Node* perception = find_fact(nodes, "Ash hid the key");
        const Node* reflected = find_fact(nodes, "I cannot trust Ash now");
        REQUIRE(prior != nullptr);
        REQUIRE(unrelated != nullptr);
        REQUIRE(perception != nullptr);
        REQUIRE(reflected != nullptr);
        REQUIRE(reflected->type == "belief");
        REQUIRE(reflected->weight == 10.0f);
        REQUIRE(reflected->entities == std::vector<std::string>{"Ash"});
        REQUIRE(perception->valid_until == 6);
        REQUIRE(prior->weight > 3.59f);
        REQUIRE(prior->weight < 3.61f);
        REQUIRE(unrelated->weight > 3.59f);
        REQUIRE(unrelated->weight < 3.61f);

        bool tension_to_prior = false;
        bool evidence_to_perception = false;
        for (const auto& edge : memory.beliefs().all_edges()) {
            if (edge.from_id == prior->id && edge.to_id == reflected->id &&
                edge.data.kind == "tension")
                tension_to_prior = true;
            if (edge.from_id == perception->id && edge.to_id == reflected->id &&
                edge.data.kind == "evidence")
                evidence_to_perception = true;
        }
        REQUIRE(tension_to_prior);
        REQUIRE(evidence_to_perception);
    }

    SECTION("throwing callback consolidates perception and decays prior") {
        CharacterMemory memory("Maren");
        memory.seed_belief("I trust Ash", {"Ash"}, 0, 4.0f);
        memory.route_fact("Ash hid the key", {"Ash"}, 5);
        memory.set_reflection_llm_callback([](const std::string&) -> std::string {
            throw std::runtime_error("reflection unavailable");
        });

        memory.reflect_perceptions(6, "A wary guard");

        const auto nodes = memory.beliefs().all_nodes(true);
        const Node* prior = find_fact(nodes, "I trust Ash");
        const Node* perception = find_fact(nodes, "Ash hid the key");
        REQUIRE(prior != nullptr);
        REQUIRE(perception != nullptr);
        REQUIRE(perception->valid_until == 6);
        REQUIRE(prior->weight > 3.59f);
        REQUIRE(prior->weight < 3.61f);
        REQUIRE(nodes.size() == 2);
    }

    SECTION("malformed response consolidates perception without adding a belief") {
        CharacterMemory memory("Maren");
        memory.route_fact("Ash hid the key", {"Ash"}, 5);
        memory.set_reflection_llm_callback([](const std::string&) {
            return std::string{"not json"};
        });

        memory.reflect_perceptions(6, "A wary guard");

        const auto nodes = memory.beliefs().all_nodes(true);
        const Node* perception = find_fact(nodes, "Ash hid the key");
        REQUIRE(perception != nullptr);
        REQUIRE(perception->valid_until == 6);
        REQUIRE(nodes.size() == 1);
    }
}

TEST_CASE("death confirmation accepts only a yes token", "[scene_loop]") {
    auto confirmed_dead = [](const std::string& response) {
        Scene scene;
        scene.scene_id = "root";
        scene.title = "Root";
        scene.system_prompt = "Narrate.";
        scene.enter_character(Character{"Guard", "A guard", false});
        Node evidence;
        evidence.fact = "Guard is dead";
        evidence.entities = {"Guard"};
        evidence.state = NodeState::Active;
        scene.world().world_graph.add_node(std::move(evidence));

        Director director(scene.world().world_graph);
        SceneLoop loop;
        loop.set_director(&director);
        loop.set_llm_callback([&](const std::string&) { return response; });
        loop.set_narrator_llm_callback(
            [](const std::string&, const std::string&, const std::string&) {
                return std::string{R"(The guard lies still.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
            });
        loop.run_player_turn(scene, "Look at the guard.");
        return scene.world().find_character("Guard")->dead;
    };

    REQUIRE(confirmed_dead("yes"));
    REQUIRE(confirmed_dead("Yes."));
    REQUIRE(confirmed_dead("YES - confirmed"));

    REQUIRE_FALSE(confirmed_dead("no"));
    REQUIRE_FALSE(confirmed_dead("yesterday he seemed dead"));
    REQUIRE_FALSE(confirmed_dead("not yes"));
}

TEST_CASE("SceneLoop keeps NPC speech out of history", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    Character npc{"Barkeep", "A gruff dwarf", false};
    scene.enter_character(std::move(npc));

    SceneLoop loop;
    loop.load_scene(scene);

    Director director(scene.world().world_graph);
    loop.set_director(&director);

    loop.set_narrator_llm_callback([&](const std::string& /*scene_id*/,
                                       const std::string& instructions,
                                       const std::string& turn_state) {
        (void)instructions;
        (void)turn_state;
        return R"(You step inside.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Barkeep","line":"What'll it be?"}],"new_characters":[],"active_cast":["Barkeep"]})";
    });

    loop.set_llm_callback([&](const std::string& prompt) {
        (void)prompt;
        return "fallback";
    });

    loop.submit_input("I enter the tavern.");

    REQUIRE(scene.history.size() == 2);
    REQUIRE(scene.dialogue.size() == 1);
    REQUIRE(scene.dialogue.messages()[0].metadata["scene_kind"] == "character");
    REQUIRE(scene.dialogue.messages()[0].content.find("What'll it be?") != std::string::npos);

    const auto& barkeep_mem = scene.world().character_memories.at("Barkeep");
    int perceptions = 0;
    barkeep_mem.beliefs().for_each([&](const Node& n) {
        if (n.type == "perception") ++perceptions;
    }, false);
    REQUIRE(perceptions == 0);

    const auto timeline = scene.display_timeline();
    REQUIRE(timeline.size() == 3);
}

TEST_CASE("SceneLoop retries Player-only speech_turns", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    Character npc{"Barkeep", "A gruff dwarf", false};
    scene.enter_character(std::move(npc));

    SceneLoop loop;
    loop.load_scene(scene);

    Director director(scene.world().world_graph);
    loop.set_director(&director);

    int narrator_calls = 0;
    loop.set_narrator_llm_callback([&](const std::string& /*scene_id*/,
                                       const std::string& instructions,
                                       const std::string& turn_state) {
        (void)instructions;
        ++narrator_calls;
        if (narrator_calls == 1) {
            REQUIRE(turn_state.find("REVISION REQUIRED") == std::string::npos);
            return R"(The barkeep watches.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Player","line":"Ale please"}],"new_characters":[],"active_cast":["Barkeep"]})";
        }
        REQUIRE(turn_state.find("REVISION REQUIRED") != std::string::npos);
        return R"(The barkeep watches.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Barkeep","line":"What'll it be?"}],"new_characters":[],"active_cast":["Barkeep"]})";
    });

    loop.set_llm_callback([](const std::string&) { return "fallback"; });

    loop.submit_input("Ale please");

    REQUIRE(narrator_calls == 2);
    REQUIRE(scene.dialogue.size() == 1);
    REQUIRE(scene.dialogue.messages()[0].metadata["speaker"] == "Barkeep");
    REQUIRE(scene.dialogue.messages()[0].content.find("What'll it be?") != std::string::npos);
}

TEST_CASE("SceneLoop allows empty speech_turns with NPCs on stage", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    Character npc{"Barkeep", "A gruff dwarf", false};
    scene.enter_character(std::move(npc));

    SceneLoop loop;
    loop.load_scene(scene);

    Director director(scene.world().world_graph);
    loop.set_director(&director);

    loop.set_narrator_llm_callback([](const std::string&, const std::string&, const std::string&) {
        return R"(You sit in silence.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Barkeep"]})";
    });

    loop.set_llm_callback([](const std::string&) { return "fallback"; });

    loop.submit_input("I wait quietly.");

    REQUIRE(scene.dialogue.size() == 0);
    REQUIRE(scene.history.size() == 2);
}

TEST_CASE("History sanitizes invalid UTF-8 on append", "[history]") {
    History h;
    SceneMessage msg;
    msg.role = Role::User;
    msg.content = std::string("hello") + '\xE2';
    h.append(msg);
    REQUIRE(h.messages()[0].content.find("hello") == 0);
    REQUIRE(h.messages()[0].content.size() > 5);
}

TEST_CASE("SceneLoop rejects input in wrong state", "[scene_loop]") {
    SceneLoop loop;
    REQUIRE_THROWS_AS(loop.submit_input("test"), std::runtime_error);
}

TEST_CASE("SceneLoop throws without callbacks", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    SceneLoop loop;
    loop.load_scene(scene);

    REQUIRE_THROWS_AS(loop.submit_input("test"), std::runtime_error);
}

TEST_CASE("SceneLoop rolls back a failed player turn", "[scene_loop][transaction]") {
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Test";
    scene.system_prompt = "Narrate.";
    scene.world().stage_exit("root", {"Existing"});

    SceneLoop loop;
    loop.load_scene(scene);
    Director director(scene.world().world_graph);
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback([](const std::string&, const std::string&,
                                      const std::string&) {
        return R"(A stranger arrives.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[{"fact":"A bell rings","type":"scene","state":"active","entities":["Bell"],"audience":[]}],"speech_turns":[],"new_characters":[{"name":"Temp","description":"A stranger"}],"active_cast":["Temp"]})";
    });
    loop.set_turn_complete_callback([](const SceneMessage&) {
        throw std::runtime_error("output failed");
    });

    REQUIRE_THROWS_AS(loop.submit_input("I listen."), std::runtime_error);
    REQUIRE(loop.state() == LoopState::WaitingForInput);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 0);
    REQUIRE(scene.dialogue.size() == 0);
    REQUIRE(scene.world().world_graph.size() == 0);
    REQUIRE(scene.world().characters.empty());
    REQUIRE(scene.world().character_memories.empty());
    REQUIRE(scene.world().pending_ops().size() == 1);
    REQUIRE(scene.world().pending_ops()[0].kind == LifecycleKind::Exit);
    REQUIRE(loop.take_last_turn_outputs().empty());
}

TEST_CASE("SceneLoop rolls back a failed autonomous cue", "[scene_loop][transaction]") {
    Scene scene;
    scene.scene_id = "away";
    scene.title = "Away";
    scene.system_prompt = "Narrate.";

    SceneLoop loop;
    loop.load_scene(scene);
    Director director(scene.world().world_graph);
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback([](const std::string&, const std::string&,
                                      const std::string&) -> std::string {
        throw std::runtime_error("narrator unavailable");
    });

    REQUIRE_THROWS_AS(loop.submit_autonomous("Advance the patrol."), std::runtime_error);
    REQUIRE(loop.state() == LoopState::WaitingForInput);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 0);
}

TEST_CASE("Rejected narrator attempts do not leak temporary minds",
          "[scene_loop][transaction]") {
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    SceneLoop loop;
    loop.load_scene(scene);
    Director director(scene.world().world_graph);
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });

    int calls = 0;
    loop.set_narrator_llm_callback([&](const std::string&, const std::string&,
                                       const std::string&) {
        ++calls;
        if (calls == 1) {
            return std::string{R"(A stranger appears.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[{"character":"Player","line":"No"}],"new_characters":[{"name":"Temp","description":"A stranger"}],"active_cast":["Temp"]})"};
        }
        return std::string{R"(The road stays empty.

<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
    });

    loop.submit_input("I wait.");
    loop.join_background();

    REQUIRE(calls == 2);
    REQUIRE(scene.world().find_character("Temp") == nullptr);
    REQUIRE(scene.world().character_memories.count("Temp") == 0);
}

TEST_CASE("Weaver work queue preserves priority and supersession behavior",
          "[weaver][work_queue]") {
    auto add_fact = [](WorldGraph& graph, const std::string& fact,
                       const std::string& entity, int turn) {
        Node node;
        node.fact = fact;
        node.entities = {entity};
        node.state = NodeState::Active;
        node.created_at = turn;
        return graph.add_node(std::move(node)).id;
    };

    SECTION("priority entity groups drain first") {
        WorldGraph graph;
        add_fact(graph, "A old", "A", 1);
        add_fact(graph, "A new", "A", 2);
        add_fact(graph, "B old", "B", 1);
        add_fact(graph, "B new", "B", 2);
        Weaver weaver(graph);
        std::vector<std::string> prompts;
        weaver.set_local_llm_callback([&](const std::string& prompt) {
            prompts.push_back(prompt);
            return std::string{R"({"superseded":[],"reason":"all current"})"};
        });

        weaver.rebuild_expiry_queue({"B"});
        REQUIRE_FALSE(weaver.expiry_queue_empty());
        REQUIRE(weaver.drain_expiry_queue(8).empty());

        REQUIRE(weaver.expiry_queue_empty());
        REQUIRE(prompts.size() == 2);
        REQUIRE(prompts[0].find("B old") != std::string::npos);
        REQUIRE(prompts[0].find("A old") == std::string::npos);
        REQUIRE(prompts[1].find("A old") != std::string::npos);
    }

    SECTION("supersession expires the old fact at the newer fact turn") {
        WorldGraph graph;
        const auto old_id = add_fact(graph, "The gate is closed", "Gate", 1);
        const auto new_id = add_fact(graph, "The gate is open", "Gate", 5);
        Weaver weaver(graph);
        std::string prompt;
        weaver.set_local_llm_callback([&](const std::string& value) {
            prompt = value;
            return std::string{"{\"superseded\":[{\"id\":"} +
                   std::to_string(old_id) + ",\"by\":" +
                   std::to_string(new_id) +
                   R"(}],"reason":"newer state"})";
        });

        weaver.rebuild_expiry_queue();
        const auto expired = weaver.drain_expiry_queue(9);

        REQUIRE(expired.size() == 1);
        REQUIRE(expired[0].id == old_id);
        REQUIRE(expired[0].reason == "newer state");
        REQUIRE(graph.get_node(old_id)->valid_until == 5);
        REQUIRE(graph.get_node(new_id)->valid_until == -1);
        REQUIRE(prompt.find("The gate is open") < prompt.find("The gate is closed"));
    }

    SECTION("missing callback consumes queued groups without graph mutations") {
        WorldGraph graph;
        const auto old_id = add_fact(graph, "The gate is closed", "Gate", 1);
        const auto new_id = add_fact(graph, "The gate is open", "Gate", 5);
        Weaver weaver(graph);

        weaver.rebuild_expiry_queue();
        REQUIRE_FALSE(weaver.expiry_queue_empty());
        REQUIRE(weaver.drain_expiry_queue(9).empty());

        REQUIRE(weaver.expiry_queue_empty());
        REQUIRE(graph.get_node(old_id)->valid_until == -1);
        REQUIRE(graph.get_node(new_id)->valid_until == -1);
    }
}

TEST_CASE("Weaver work queue stop leaves remaining groups queued",
          "[weaver][work_queue]") {
    using namespace std::chrono_literals;
    WorldGraph graph;
    auto add_fact = [&](const std::string& fact, const std::string& entity, int turn) {
        Node node;
        node.fact = fact;
        node.entities = {entity};
        node.state = NodeState::Active;
        node.created_at = turn;
        graph.add_node(std::move(node));
    };
    add_fact("A old", "A", 1);
    add_fact("A new", "A", 2);
    add_fact("B old", "B", 1);
    add_fact("B new", "B", 2);

    Weaver weaver(graph);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_local_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"superseded":[],"reason":"all current"})"};
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

TEST_CASE("load_scene waits for background work on the previous scene",
          "[scene_loop][background]") {
    using namespace std::chrono_literals;
    Scene first;
    first.scene_id = "first";
    first.title = "First";
    first.system_prompt = "Narrate.";
    for (const char* fact : {"The gate is shut", "The torch is lit"}) {
        Node node;
        node.fact = fact;
        first.world().world_graph.add_node(std::move(node));
    }
    Scene second;
    second.scene_id = "second";

    Weaver weaver(first.world().world_graph);
    weaver.set_interval(1);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });

    SceneLoop loop;
    loop.load_scene(first);
    Director director(first.world().world_graph);
    loop.set_director(&director);
    loop.set_weaver(&weaver);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback([](const std::string&, const std::string&,
                                      const std::string&) {
        return std::string{R"(Time passes.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
    });
    loop.submit_input("Wait.");
    started.get_future().wait();

    auto switching = std::async(std::launch::async, [&] { loop.load_scene(second); });
    REQUIRE(switching.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(switching.wait_for(2s) == std::future_status::ready);
    switching.get();
    REQUIRE(loop.state() == LoopState::WaitingForInput);
}

TEST_CASE("SceneLoop destruction waits for active background work",
          "[scene_loop][background]") {
    using namespace std::chrono_literals;
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Root";
    scene.system_prompt = "Narrate.";
    for (const char* fact : {"The gate is shut", "The torch is lit"}) {
        Node node;
        node.fact = fact;
        scene.world().world_graph.add_node(std::move(node));
    }

    Weaver weaver(scene.world().world_graph);
    weaver.set_interval(1);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });
    Director director(scene.world().world_graph);
    auto loop = std::make_unique<SceneLoop>();
    loop->load_scene(scene);
    loop->set_director(&director);
    loop->set_weaver(&weaver);
    loop->set_llm_callback([](const std::string&) { return "fallback"; });
    loop->set_narrator_llm_callback([](const std::string&, const std::string&,
                                       const std::string&) {
        return std::string{R"(Time passes.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
    });
    loop->submit_input("Wait.");
    started.get_future().wait();

    auto destroying = std::async(std::launch::async, [&] { loop.reset(); });
    REQUIRE(destroying.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(destroying.wait_for(2s) == std::future_status::ready);
    destroying.get();
}

TEST_CASE("Story waits for background mutations before returning and saving",
          "[story][background]") {
    using namespace std::chrono_literals;
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";
    Node first;
    first.fact = "The gate is shut";
    const auto first_id = root.world().world_graph.add_node(std::move(first)).id;
    Node second;
    second.fact = "The guard has a key";
    const auto second_id = root.world().world_graph.add_node(std::move(second)).id;

    Story story = Story::from_scene(std::move(root));
    Director director(story.world().world_graph);
    Weaver weaver(story.world().world_graph);
    weaver.set_interval(1);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_weaver(&weaver);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback([](const std::string&, const std::string&,
                                      const std::string&) {
        return std::string{R"(The guard approaches.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
    });

    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    std::string weave_prompt;
    weaver.set_llm_callback([&](const std::string& prompt) {
        weave_prompt = prompt;
        started.set_value();
        release_future.wait();
        return std::string{"{\"connect\":[{\"from\":"} + std::to_string(first_id) +
               ",\"to\":" + std::to_string(second_id) +
               R"(,"weight":0.8,"reason":"key"}],"disconnect":[],"reweight":[]})";
    });

    const auto save_dir = std::filesystem::temp_directory_path() /
        ("rhapsode-phase1-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    story.bind_runtime(loop);
    story.set_saves_dir(save_dir.string());

    auto advancing = std::async(std::launch::async, [&] {
        return story.advance_scene("I watch.");
    });
    started.get_future().wait();
    REQUIRE(advancing.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(advancing.wait_for(2s) == std::future_status::ready);
    advancing.get();

    REQUIRE(weave_prompt.find("[1]") != std::string::npos);
    REQUIRE(weave_prompt.find("[2]") != std::string::npos);
    REQUIRE(story.world().world_graph.all_edges().size() == 1);
    std::ifstream saved(save_dir / "world.json");
    json saved_json;
    saved >> saved_json;
    World restored = World::from_json(saved_json);
    REQUIRE(restored.world_graph.all_edges().size() == 1);
    saved.close();
    std::filesystem::remove_all(save_dir);
}

TEST_CASE("Story keeps player outputs separate from a scheduled off-stage turn",
          "[story][scene_loop][characterization]") {
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";

    Story story = Story::from_scene(std::move(root));
    Scene* player_scene = story.get_scene("root");
    Scene* off_stage = story.fork_scene("root", "away", {}, "Advance the patrol");
    REQUIRE(player_scene != nullptr);
    REQUIRE(off_stage != nullptr);
    REQUIRE(&player_scene->world().world_graph == &off_stage->world().world_graph);

    Director director(story.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string& scene_id, const std::string&, const std::string&) {
            const std::string narration = scene_id == "root"
                ? "The player scene advances."
                : "The distant patrol advances.";
            const std::string fact = scene_id == "root"
                ? "The player opens the gate"
                : "The patrol reaches the ridge";
            return narration + R"(
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[{"fact":")" + fact +
                   R"(","entities":[]}],"speech_turns":[],"new_characters":[],"active_cast":[]})";
        });
    story.bind_runtime(loop);
    story.set_scheduler_callback([](const std::string&, const std::string&) {
        return std::string{"away"};
    });

    const auto outputs = story.advance_scene("Open the gate.");

    REQUIRE(outputs.size() == 1);
    REQUIRE(outputs[0].content == "The player scene advances.");
    REQUIRE(player_scene->history.messages().back().content == "The player scene advances.");
    REQUIRE(off_stage->history.messages().back().content == "The distant patrol advances.");

    // Story consumes each explicit result, so its later off-stage turn cannot
    // leak through the legacy SceneLoop result cache.
    REQUIRE(loop.last_director_output().new_nodes.empty());
    REQUIRE(loop.take_completed_expiry_ops().empty());
}

TEST_CASE("Story persists lifecycle changes after the completed player beat",
          "[story][persistence][characterization]") {
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";
    root.enter_character(Character{"Scout", "A careful scout", false});
    Node first;
    first.fact = "The ridge is visible";
    const auto first_id = root.world().world_graph.add_node(std::move(first)).id;
    Node second;
    second.fact = "The scout carries a spyglass";
    const auto second_id = root.world().world_graph.add_node(std::move(second)).id;

    Story story = Story::from_scene(std::move(root));
    Director director(story.world().world_graph);
    Weaver weaver(story.world().world_graph);
    weaver.set_interval(1);
    weaver.set_llm_callback([&](const std::string&) {
        return std::string{"{\"connect\":[{\"from\":"} + std::to_string(first_id) +
               ",\"to\":" + std::to_string(second_id) +
               R"(,"weight":0.8,"reason":"spyglass"}],"disconnect":[],"reweight":[]})";
    });
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_weaver(&weaver);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(The scout prepares to leave.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":["Scout"]})"};
        });
    story.bind_runtime(loop);
    story.set_lifecycle_callback([](const std::string&, const std::string&) {
        return std::string{R"({"fork":{"cast":["Scout"],"driving_intention":"Scout the ridge"},"merge_into":null,"conclude":null,"exited":[]})"};
    });

    const auto save_dir = std::filesystem::temp_directory_path() /
        ("rhapsode-story-lifecycle-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    story.set_saves_dir(save_dir.string());

    story.advance_scene("Send the scout ahead.");

    REQUIRE(story.scene_count() == 2);
    REQUIRE(story.get_scene("root_f0_0") != nullptr);
    REQUIRE(std::filesystem::exists(save_dir / "story.json"));
    REQUIRE(std::filesystem::exists(save_dir / "root_f0_0.json"));

    std::ifstream manifest_file(save_dir / "story.json");
    json manifest;
    manifest_file >> manifest;
    REQUIRE(manifest["scene_ids"] == json::array({"root", "root_f0_0"}));

    std::ifstream world_file(save_dir / "world.json");
    json world_json;
    world_file >> world_json;
    World restored = World::from_json(world_json);
    const Character* scout = restored.find_character("Scout");
    REQUIRE(scout != nullptr);
    REQUIRE(scout->in_scene("root_f0_0"));
    REQUIRE_FALSE(scout->in_scene("root"));
    REQUIRE(restored.world_graph.all_edges().size() == 1);

    manifest_file.close();
    world_file.close();
    std::filesystem::remove_all(save_dir);
}

TEST_CASE("Story undo persists and reuses the bound SceneLoop",
          "[story][undo][persistence]") {
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";

    Story story = Story::from_scene(std::move(root));
    Director director(story.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(The moment advances.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
        });

    const auto save_dir = std::filesystem::temp_directory_path() /
        ("rhapsode-story-undo-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    story.bind_runtime(loop);
    story.set_saves_dir(save_dir.string());

    story.advance_scene("Go forward.");
    REQUIRE(story.active_scene()->turn_index == 1);
    REQUIRE(story.revert_active_turns(1) == 1);
    REQUIRE(story.active_scene()->turn_index == 0);

    std::ifstream reverted_file(save_dir / "root.json");
    json reverted_json;
    reverted_file >> reverted_json;
    REQUIRE(reverted_json["turn_index"] == 0);
    reverted_file.close();

    story.advance_scene("Try again.");
    REQUIRE(story.active_scene()->turn_index == 1);
    std::filesystem::remove_all(save_dir);
}

TEST_CASE("Story load waits for bound runtime work before rebuilding scenes",
          "[story][background][persistence]") {
    using namespace std::chrono_literals;
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";
    for (const char* fact : {"The gate is shut", "The guard has a key"}) {
        Node node;
        node.fact = fact;
        root.world().world_graph.add_node(std::move(node));
    }

    Story story = Story::from_scene(std::move(root));
    const auto save_dir = std::filesystem::temp_directory_path() /
        ("rhapsode-story-load-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    story.save(save_dir.string());

    Director director(story.world().world_graph);
    Weaver weaver(story.world().world_graph);
    weaver.set_interval(1);
    std::promise<void> started;
    std::promise<void> release;
    auto release_future = release.get_future().share();
    weaver.set_llm_callback([&](const std::string&) {
        started.set_value();
        release_future.wait();
        return std::string{R"({"connect":[],"disconnect":[],"reweight":[]})"};
    });

    SceneLoop loop;
    loop.set_director(&director);
    loop.set_weaver(&weaver);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Time passes.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
        });
    story.bind_runtime(loop);
    loop.load_scene(*story.active_scene());
    loop.submit_input("Wait.");
    started.get_future().wait();

    auto loading = std::async(std::launch::async, [&] {
        story.load_save(save_dir.string());
    });
    REQUIRE(loading.wait_for(20ms) == std::future_status::timeout);
    release.set_value();
    REQUIRE(loading.wait_for(2s) == std::future_status::ready);
    loading.get();

    REQUIRE(story.active_scene() != nullptr);
    REQUIRE(story.active_scene()->turn_index == 0);
    std::filesystem::remove_all(save_dir);
}

TEST_CASE("SceneLoop returns one complete result and detaches the Scene",
          "[scene_loop][result]") {
    Scene root;
    root.scene_id = "root";
    root.title = "Root";
    root.system_prompt = "Narrate.";

    Scene second = root.fork("second", {});
    Director director(root.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string& scene_id, const std::string&, const std::string&) {
            return std::string{"Narration for "} + scene_id + R"(.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[{"fact":"Fact for )" + scene_id +
                   R"(","entities":[]}],"speech_turns":[],"new_characters":[],"active_cast":[]})";
        });

    SceneTurnResult first = loop.run_player_turn(root, "Act.");
    REQUIRE(loop.state() == LoopState::Idle);
    REQUIRE(first.scene_id == "root");
    REQUIRE(first.completed_turn == 1);
    REQUIRE(first.outputs.size() == 1);
    REQUIRE(first.outputs[0].content == "Narration for root.");
    REQUIRE(first.director.new_nodes.size() == 1);
    REQUIRE(first.director.new_nodes[0].fact == "Fact for root");
    REQUIRE(first.expiry.empty());
    REQUIRE(loop.take_last_turn_outputs().empty());
    REQUIRE(loop.take_completed_expiry_ops().empty());
    REQUIRE(loop.last_director_output().new_nodes.empty());

    SceneTurnResult next = loop.run_autonomous_turn(second, "Continue.");
    REQUIRE(loop.state() == LoopState::Idle);
    REQUIRE(next.scene_id == "second");
    REQUIRE(next.completed_turn == 1);
    REQUIRE(next.outputs.size() == 1);
    REQUIRE(next.outputs[0].content == "Narration for second.");
}

TEST_CASE("SceneLoop detaches the Scene when synchronous turn execution fails",
          "[scene_loop][result][transaction]") {
    Scene scene;
    scene.scene_id = "root";
    scene.title = "Root";
    scene.system_prompt = "Narrate.";

    Director director(scene.world().world_graph);
    SceneLoop loop;
    loop.set_director(&director);
    loop.set_llm_callback([](const std::string&) { return "fallback"; });
    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) -> std::string {
            throw std::runtime_error("narrator unavailable");
        });

    REQUIRE_THROWS_AS(loop.run_player_turn(scene, "Act."), std::runtime_error);
    REQUIRE(loop.state() == LoopState::Idle);
    REQUIRE(scene.turn_index == 0);
    REQUIRE(scene.history.size() == 0);

    loop.set_narrator_llm_callback(
        [](const std::string&, const std::string&, const std::string&) {
            return std::string{R"(Recovered.
<<<RHAPSODE_JSON>>>
{"transitions":[],"new_nodes":[],"speech_turns":[],"new_characters":[],"active_cast":[]})"};
        });
    const SceneTurnResult recovered = loop.run_player_turn(scene, "Try again.");
    REQUIRE(recovered.outputs.size() == 1);
    REQUIRE(recovered.outputs[0].content == "Recovered.");
    REQUIRE(loop.state() == LoopState::Idle);
}

TEST_CASE("SceneLoop rejects runtime services bound to another WorldGraph",
          "[scene_loop][ownership]") {
    Scene configured;
    configured.scene_id = "configured";
    Scene other;
    other.scene_id = "other";

    SECTION("Director mismatch") {
        Director director(configured.world().world_graph);
        SceneLoop loop;
        loop.set_director(&director);

        try {
            loop.run_player_turn(other, "Act.");
            FAIL("Expected mismatched Director graph to be rejected");
        } catch (const std::runtime_error& error) {
            REQUIRE(std::string{error.what()} ==
                    "Director is bound to a different WorldGraph");
        }
        REQUIRE(loop.state() == LoopState::Idle);
        REQUIRE(other.history.size() == 0);
        REQUIRE(other.turn_index == 0);
    }

    SECTION("Weaver mismatch") {
        Director director(configured.world().world_graph);
        Weaver weaver(other.world().world_graph);
        SceneLoop loop;
        loop.set_director(&director);
        loop.set_weaver(&weaver);

        try {
            loop.run_player_turn(configured, "Act.");
            FAIL("Expected mismatched Weaver graph to be rejected");
        } catch (const std::runtime_error& error) {
            REQUIRE(std::string{error.what()} ==
                    "Weaver is bound to a different WorldGraph");
        }
        REQUIRE(loop.state() == LoopState::Idle);
        REQUIRE(configured.history.size() == 0);
        REQUIRE(configured.turn_index == 0);
    }
}
