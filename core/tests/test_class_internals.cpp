#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "rhapsode/annotator.h"
#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/graph_plan.h"
#include "rhapsode/llm_callback.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/node.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

using namespace rhapsode;
using json = nlohmann::json;

namespace {

Node fact(std::string text, std::string entity, int turn) {
    Node node;
    node.fact = std::move(text);
    node.entities = {std::move(entity)};
    node.state = NodeState::Active;
    node.created_at = turn;
    return node;
}

}  // namespace

TEST_CASE("WorldGraph edge mutation accepts either endpoint order",
          "[world_graph][characterization]") {
    WorldGraph graph;
    const auto older = graph.add_node(fact("Gate shut", "Gate", 1)).id;
    const auto newer = graph.add_node(fact("Gate open", "Gate", 2)).id;

    REQUIRE(graph.add_relation(newer, older, 0.5f, 2, "chain"));
    REQUIRE(graph.set_edge_weight(newer, older, 0.75f));
    REQUIRE(graph.set_edge_kind(newer, older, "evidence"));
    REQUIRE(graph.set_edge_active(newer, older, false));

    const auto edges = graph.all_edges();
    REQUIRE(edges.size() == 1);
    REQUIRE(edges.front().from_id == older);
    REQUIRE(edges.front().to_id == newer);
    REQUIRE(edges.front().data.weight == 0.75f);
    REQUIRE(edges.front().data.kind == "evidence");
    REQUIRE_FALSE(edges.front().data.active);
}

TEST_CASE("WorldGraph thread queries share active-edge semantics",
          "[world_graph][characterization]") {
    WorldGraph graph;
    const auto a = graph.add_node(fact("A", "A", 1)).id;
    const auto b = graph.add_node(fact("B", "B", 2)).id;
    const auto c = graph.add_node(fact("C", "C", 3)).id;
    REQUIRE(graph.add_relation(a, b));

    REQUIRE(graph.thread_containing(a) == std::vector<std::uint64_t>{a, b});
    auto threads = graph.all_threads();
    REQUIRE(threads.size() == 2);
    REQUIRE(graph.set_edge_active(a, b, false));
    REQUIRE(graph.thread_containing(a) == std::vector<std::uint64_t>{a});
    REQUIRE(graph.thread_containing(c) == std::vector<std::uint64_t>{c});
}

TEST_CASE("Graph plan result retains expired and new nodes",
          "[director][characterization]") {
    WorldGraph graph;
    Node existing = fact("Gate shut", "Gate", 1);
    existing.active_ctx = "The gate blocks the road.";
    const auto existing_id = graph.add_node(std::move(existing)).id;

    const json plan = {
        {"transitions", json::array({{
            {"id", existing_id}, {"state", "resolved"},
        }})},
        {"new_nodes", json::array({{
            {"fact", "Gate open"}, {"state", "active"},
            {"active_ctx", "The road is clear."},
            {"entities", json::array({"Gate"})},
        }})},
    };

    const GraphPlanResult output = apply_graph_plan(graph, 4, plan);

    REQUIRE(output.newly_expired.size() == 1);
    REQUIRE(output.newly_expired.front().id == existing_id);
    REQUIRE(output.new_nodes.size() == 1);
    REQUIRE(output.new_nodes.front().fact == "Gate open");
    REQUIRE(output.new_nodes.front().active_ctx == "The road is clear.");
}

TEST_CASE("CharacterMemory monologue update writes a private line",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.apply_perception_json(1, R"({"perception":"The gate swings open."})");

    memory.update_monologues(1, "Careful",
        [](const std::string&) {
            return std::string{
                R"({"line":"Finally — a way out.","knows":[{"fact":"I can leave.","entities":["Gate"]}]})"};
        });

    REQUIRE(memory.monologue_lines().size() == 1);
    REQUIRE(memory.monologue_lines().front().text.find("way out") != std::string::npos);
    REQUIRE(memory.monologue_lines().front().turn == 1);
    REQUIRE(memory.monologue_turn() == 1);
    const std::string view = memory.render_thoughts({"Gate"});
    REQUIRE(view.find("I can leave.") == std::string::npos);
}

TEST_CASE("CharacterMemory perception prompt user is the narration window",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    const std::string window = "The gate swings open.";
    const std::string prompt =
        memory.build_perception_prompt(window, "I keep the watch.");
    const std::string sentinel = "<<<RHAPSODE_PERCEPTION_USER>>>";
    const auto split = prompt.find(sentinel);
    REQUIRE(split != std::string::npos);
    const std::string system = prompt.substr(0, split);
    std::string user = prompt.substr(split + sentinel.size());
    if (!user.empty() && user.front() == '\n')
        user.erase(user.begin());
    REQUIRE(system.find("You are Scout") != std::string::npos);
    REQUIRE(system.find("Who you are:") != std::string::npos);
    REQUIRE(system.find("I keep the watch.") != std::string::npos);
    REQUIRE(system.find("\"perception\"") != std::string::npos);
    REQUIRE(user == window);
    REQUIRE(user.find("I keep the watch.") == std::string::npos);
    REQUIRE(user.find("[1 take]") == std::string::npos);
}

TEST_CASE("CharacterMemory apply_perception overwrites text and still advances on empty",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.apply_perception_json(
        1,
        R"({"perception":"The latch gave.","facts":[{"fact":"The gate is open","entities":["Gate"]}]})");
    REQUIRE(memory.perception() == "The latch gave.");
    REQUIRE(memory.perception_turn() == 1);
    REQUIRE(memory.render_thoughts({"Gate"}).find("The gate is open") != std::string::npos);

    memory.apply_perception_json(2, R"({"perception":"A bird lands on the wall."})");
    REQUIRE(memory.perception() == "A bird lands on the wall.");
    REQUIRE(memory.perception_turn() == 2);

    memory.apply_perception_json(3, R"({"perception":null})");
    REQUIRE(memory.perception() == "A bird lands on the wall.");
    REQUIRE(memory.perception_turn() == 3);

    memory.apply_perception_json(4, R"({"perception":""})");
    REQUIRE(memory.perception() == "A bird lands on the wall.");
    REQUIRE(memory.perception_turn() == 4);

    const auto query = memory.render_mind_query();
    REQUIRE(query["perception"] == "A bird lands on the wall.");
}

TEST_CASE("CharacterMemory monologue prompt copies perception not narration",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    const std::string window = "NARRATION_WINDOW_UNIQUE The gate swings open.";
    memory.update_perception(1, "I keep the watch.", window,
        [](const std::string& prompt) {
            REQUIRE(prompt.find("<<<RHAPSODE_PERCEPTION_USER>>>")
                    != std::string::npos);
            REQUIRE(prompt.find("NARRATION_WINDOW_UNIQUE") != std::string::npos);
            REQUIRE(prompt.find("[1 take]") == std::string::npos);
            return std::string{R"({"perception":"The latch clicked."})"};
        });

    const std::string sentinel = "<<<RHAPSODE_MONOLOGUE_USER>>>";
    const auto user_of = [&](const std::string& prompt) {
        const auto split = prompt.find(sentinel);
        REQUIRE(split != std::string::npos);
        return prompt.substr(split + sentinel.size());
    };

    std::string first;
    memory.update_monologues(
        1, "Careful",
        [&](const std::string& prompt) {
            first = prompt;
            return std::string{R"({"line":null})"};
        });

    const std::string system = first.substr(0, first.find(sentinel));
    const std::string user = user_of(first);

    REQUIRE(system.find("\"line\"") != std::string::npos);
    REQUIRE(system.find("You are Scout") == std::string::npos);
    REQUIRE(system.find("The latch clicked") == std::string::npos);

    const auto name_at = user.find("You are Scout");
    const auto bible_at = user.find("Who you are:");
    REQUIRE(name_at != std::string::npos);
    REQUIRE(bible_at != std::string::npos);
    REQUIRE(name_at < bible_at);
    REQUIRE(user.find("I keep the watch.") != std::string::npos);
    REQUIRE(user.find("The latch clicked.") != std::string::npos);
    REQUIRE(user.find(window) == std::string::npos);
    REQUIRE(user.find("NARRATION_WINDOW_UNIQUE") == std::string::npos);
    REQUIRE(user.find("[1 take]") == std::string::npos);
    REQUIRE(user.find("What just happened:") == std::string::npos);
    REQUIRE(memory.monologue_lines().empty());
}

TEST_CASE("CharacterMemory private lines stay a prefix when perception changes",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.apply_perception_json(1, R"({"perception":"The gate swings open."})");
    const std::string sentinel = "<<<RHAPSODE_MONOLOGUE_USER>>>";
    const auto user_of = [&](const std::string& prompt) {
        const auto split = prompt.find(sentinel);
        REQUIRE(split != std::string::npos);
        return prompt.substr(split + sentinel.size());
    };

    std::string first;
    memory.update_monologues(
        1, "Careful",
        [&](const std::string& prompt) {
            first = prompt;
            return std::string{R"({"line":"Finally — a way out."})"};
        });

    memory.apply_perception_json(2, R"({"perception":"A bird lands on the wall."})");
    std::string second;
    memory.update_monologues(
        2, "Careful",
        [&](const std::string& prompt) {
            second = prompt;
            return std::string{R"({"line":null})"};
        });

    REQUIRE(second.substr(0, second.find(sentinel)) ==
            first.substr(0, first.find(sentinel)));
    const std::string user = user_of(first);
    const std::string user2 = user_of(second);
    REQUIRE(user.find("The gate swings open.") != std::string::npos);
    REQUIRE(user.find("Finally — a way out.") == std::string::npos);
    REQUIRE(user2.find("Finally — a way out.") != std::string::npos);
    REQUIRE(user2.find("A bird lands on the wall.") != std::string::npos);
    REQUIRE(user2.find("The gate swings open.") == std::string::npos);
    const auto thought_at = user2.find("Finally — a way out.");
    const auto bird_at = user2.find("A bird lands on the wall.");
    REQUIRE(thought_at < bird_at);
    const std::string history = user2.substr(0, thought_at);
    REQUIRE(user.find(history) == 0);
}

TEST_CASE("CharacterMemory monologue persists and old streams flatten",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.apply_perception_json(1, R"({"perception":"The gate swings open."})");
    memory.update_monologues(
        1, "Careful",
        [](const std::string&) {
            return std::string{R"({"line":"Keep still."})"};
        });
    const auto saved = memory.to_json();
    REQUIRE(saved.contains("monologue"));
    REQUIRE_FALSE(saved.contains("streams"));
    REQUIRE_FALSE(saved.contains("objective_journal"));
    REQUIRE_FALSE(saved.contains("line_cnt"));
    REQUIRE_FALSE(saved.contains("stream_cnt"));
    REQUIRE(saved["perception"] == "The gate swings open.");
    REQUIRE(saved["perception_turn"] == 1);
    REQUIRE(saved["monologue_turn"] == 1);
    REQUIRE(saved["monologue"][0]["text"] == "Keep still.");
    REQUIRE_FALSE(saved["monologue"][0].contains("after"));

    const CharacterMemory loaded = CharacterMemory::from_json(saved);
    REQUIRE(loaded.monologue_lines().size() == 1);
    REQUIRE(loaded.monologue_lines().front().text == "Keep still.");
    REQUIRE(loaded.perception() == "The gate swings open.");
    REQUIRE(loaded.perception_turn() == 1);

    json legacy = {
        {"name", "Scout"},
        {"streams", json::array({
            {{"id", "self"},
             {"lines", json::array({
                 {{"turn", 1}, {"text", "If that latch gives, I run."}, {"seq", 2}},
                 {{"turn", 1}, {"text", "Keep still."}, {"seq", 1}},
             })}},
        })},
    };
    const CharacterMemory flattened = CharacterMemory::from_json(legacy);
    REQUIRE(flattened.monologue_lines().size() == 2);
    REQUIRE(flattened.monologue_lines()[0].text == "Keep still.");
    REQUIRE(flattened.monologue_lines()[1].text.find("latch") != std::string::npos);
}

TEST_CASE("CharacterMemory ignores legacy objective_journal keys",
          "[character_memory][characterization]") {
    json legacy = {
        {"name", "Scout"},
        {"objective_journal", json::array({
            {{"turn", 1}, {"type", "take"}, {"text", "The gate swings open."}},
            {{"turn", 1}, {"type", "seen"}, {"text", "The latch gave."}},
        })},
        {"observation_consumed_lines", 2},
        {"monologue", json::array({
            {{"turn", 1}, {"text", "Keep still."}, {"after", 2}},
        })},
    };
    const CharacterMemory loaded = CharacterMemory::from_json(legacy);
    REQUIRE(loaded.perception().empty());
    REQUIRE(loaded.perception_turn() == -1);
    REQUIRE(loaded.monologue_lines().size() == 1);
    REQUIRE(loaded.monologue_lines().front().text == "Keep still.");
    REQUIRE(loaded.monologue_lines().front().turn == 1);
}

TEST_CASE("Perception claims by scene turn; monologue waits for that apply",
          "[world][character_memory]") {
    World world;
    Character alice{"Alice", "A", false};
    alice.core = "I keep the watch.";
    world.enter_character("root", std::move(alice));
    std::vector<PromptJob> perc_jobs;
    std::vector<PromptJob> mono_jobs;
    const auto none_ready =
        [](std::size_t, int, std::string&, bool&) { return false; };

    world.poll_perceptions(
        "root", 1, "The bell rings.", none_ready,
        [&](const std::vector<PromptJob>& jobs) {
            perc_jobs.insert(perc_jobs.end(), jobs.begin(), jobs.end());
        });
    REQUIRE(perc_jobs.size() == 1);
    REQUIRE(perc_jobs.front().prompt.find("Who you are:") != std::string::npos);
    REQUIRE(perc_jobs.front().prompt.find("I keep the watch.") != std::string::npos);
    REQUIRE(perc_jobs.front().prompt.find("The bell rings.") != std::string::npos);

    world.poll_monologues(
        "root", 1, none_ready,
        [&](const std::vector<PromptJob>& jobs) {
            mono_jobs.insert(mono_jobs.end(), jobs.begin(), jobs.end());
        });
    REQUIRE(mono_jobs.empty());
    REQUIRE(world.character_memories().at("Alice").perception_turn() == -1);

    world.apply_ready_perceptions(
        1,
        [](std::size_t, int, std::string& raw, bool& failed) {
            raw = R"({"perception":"Alice hears the bell."})";
            failed = false;
            return true;
        });
    REQUIRE(world.character_memories().at("Alice").perception()
            == "Alice hears the bell.");
    REQUIRE(world.character_memories().at("Alice").perception_turn() == 1);

    world.poll_monologues(
        "root", 1, none_ready,
        [&](const std::vector<PromptJob>& jobs) {
            mono_jobs.insert(mono_jobs.end(), jobs.begin(), jobs.end());
        });
    REQUIRE(mono_jobs.size() == 1);
    REQUIRE(mono_jobs.front().prompt.find("Alice hears the bell.")
            != std::string::npos);
    REQUIRE(mono_jobs.front().prompt.find("The bell rings.") == std::string::npos);

    perc_jobs.clear();
    world.poll_perceptions(
        "root", 1, "The bell rings.", none_ready,
        [&](const std::vector<PromptJob>& jobs) {
            perc_jobs.insert(perc_jobs.end(), jobs.begin(), jobs.end());
        });
    REQUIRE(perc_jobs.empty());
}

TEST_CASE("MemorySystem preserves callback payloads and query results",
          "[memory_system][characterization]") {
    MemorySystem memory("root");
    std::string stored_id;
    json stored_metadata;
    std::string deleted_ids;
    memory.set_embed_callback([](const std::string&) { return std::string{"[0.5]"}; });
    memory.set_store_callback(
        [&](const std::string& collection, const std::string& id,
            const std::string&, const std::string& embedding,
            const std::string& metadata) {
            REQUIRE(collection == "root_nodes");
            REQUIRE(embedding == "[0.5]");
            stored_id = id;
            stored_metadata = json::parse(metadata);
        });
    memory.set_query_callback(
        [](const std::string&, const std::string&, int,
           const std::string&) {
            return std::string{
                R"({"ids":[["node_7"]],"metadatas":[[{"node_id":7}]]})"};
        });
    memory.set_delete_callback(
        [&](const std::string&, const std::string& ids) { deleted_ids = ids; });

    memory.store_node(7, "Gate open", "active", "fact", 3);
    REQUIRE(stored_id == "node_7");
    REQUIRE(stored_metadata["node_id"] == 7);
    REQUIRE(stored_metadata["created_at"] == 3);
    REQUIRE(memory.search_nodes("gate") == std::vector<std::uint64_t>{7});
    memory.delete_nodes({7});
    REQUIRE(json::parse(deleted_ids) == json::array({"node_7"}));
}

TEST_CASE("Annotator keeps roster spans over overlapping NER spans",
          "[annotator][characterization]") {
    World world;
    world.enter_character("root", Character{"Scout", "Careful", false});
    Annotator annotator(world);
    annotator.set_ner_callback([](const std::string&) {
        return std::string{
            R"([{"start":0,"end":5,"text":"Scout","category":"person"},{"start":6,"end":11,"text":"waits","category":"event"}])"};
    });

    const auto spans = annotator.annotate("Scout waits");

    REQUIRE(spans.size() == 2);
    REQUIRE(spans[0].text == "Scout");
    REQUIRE(spans[0].category == "character");
    REQUIRE(spans[1].text == "waits");
    REQUIRE(spans[1].category == "event");
}
