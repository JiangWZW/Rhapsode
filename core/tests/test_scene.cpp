#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "rhapsode/scene_message.h"
#include "rhapsode/history.h"
#include "rhapsode/character.h"
#include "rhapsode/scene.h"
#include "rhapsode/director.h"
#include "rhapsode/scene_loop.h"

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
    scene.characters.push_back({"Player", "An adventurer", true});
    scene.characters.push_back({"NPC", "A villager", false});

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
    REQUIRE(restored.characters.size() == 2);
    REQUIRE(restored.characters[0].is_player == true);
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

    Director director(scene.world_graph);
    loop.set_director(&director);

    loop.set_narrator_llm_callback([&](const std::string& instructions,
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

TEST_CASE("SceneLoop keeps NPC speech out of history", "[scene_loop]") {
    Scene scene;
    scene.title = "Test";
    scene.system_prompt = "Narrate.";

    Character npc{"Barkeep", "A gruff dwarf", false};
    npc.on_stage = true;
    scene.enter_character(std::move(npc));

    SceneLoop loop;
    loop.load_scene(scene);

    Director director(scene.world_graph);
    loop.set_director(&director);

    loop.set_narrator_llm_callback([&](const std::string& instructions,
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

    const auto timeline = scene.display_timeline();
    REQUIRE(timeline.size() == 3);
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
