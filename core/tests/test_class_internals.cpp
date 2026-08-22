#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "rhapsode/annotator.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/graph_plan.h"
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

TEST_CASE("Graph plan result retains graph changes and context",
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
    REQUIRE(output.context_blocks == std::vector<std::string>{"The road is clear."});
}

TEST_CASE("CharacterMemory monologue update writes knows and stream lines",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.seed_belief("The gate is shut", {"Gate"}, 0);
    memory.route_fact("The gate has opened", {"Gate"}, 1);

    memory.update_monologues(1, "Careful", "The gate swings open.",
        [](const std::string&) {
            return std::string{
                R"({"appends":[{"stream_id":"self","text":"Finally — a way out."}],"ops":[],"knows":[{"fact":"I can finally leave.","entities":["Gate"],"weight":7,"relation":"evidence"}],"core_revision":null})"};
        });

    const std::string view = memory.view_of({"Gate"});
    REQUIRE(view.find("The gate is shut") != std::string::npos);
    REQUIRE(view.find("I can finally leave.") != std::string::npos);
    REQUIRE(memory.active_stream_count() >= 1);
    bool has_line = false;
    for (const auto& stream : memory.streams()) {
        if (stream.id != "self") continue;
        for (const auto& line : stream.lines) {
            if (line.text.find("way out") != std::string::npos)
                has_line = true;
        }
    }
    REQUIRE(has_line);
    for (const auto& node : memory.beliefs().all_nodes())
        REQUIRE(node.type != "perception");
}

TEST_CASE("CharacterMemory monologue prompt puts identity before this take",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.seed_belief("The gate is shut", {"Gate"}, 0);
    memory.route_fact("The gate has opened", {"Gate"}, 1);

    const char* kListen =
        R"({"appends":[],"ops":[],"knows":[],"core_revision":null})";
    const std::string voice = "  Voice: clipped and dry\n";
    std::string first;
    memory.update_monologues(
        1, "Careful", "The gate swings open.",
        [&](const std::string& prompt) {
            first = prompt;
            return std::string{kListen};
        },
        voice);

    const std::string sentinel = "<<<RHAPSODE_MONOLOGUE_USER>>>";
    const auto split = first.find(sentinel);
    REQUIRE(split != std::string::npos);
    const std::string system = first.substr(0, split);
    const std::string user = first.substr(split + sentinel.size());

    REQUIRE(system.find("\"appends\"") != std::string::npos);
    REQUIRE(system.find("You are Scout") == std::string::npos);
    REQUIRE(system.find("The gate swings open") == std::string::npos);

    const auto name_at = user.find("You are Scout");
    const auto bible_at = user.find("I keep the watch.");
    const auto voice_at = user.find("clipped and dry");
    const auto wants_at = user.find("Through-lines you are already carrying");
    const auto beats_at = user.find("Recent inner beats");
    const auto truths_at = user.find("What you already hold as true");
    const auto take_at = user.find("What just happened");
    const auto raw_at = user.find("What reached you this take");
    REQUIRE(name_at != std::string::npos);
    REQUIRE(bible_at != std::string::npos);
    REQUIRE(voice_at != std::string::npos);
    REQUIRE(wants_at != std::string::npos);
    REQUIRE(beats_at != std::string::npos);
    REQUIRE(truths_at != std::string::npos);
    REQUIRE(take_at != std::string::npos);
    REQUIRE(raw_at != std::string::npos);
    REQUIRE(name_at < bible_at);
    REQUIRE(voice_at < take_at);
    REQUIRE(bible_at < wants_at);
    REQUIRE(wants_at < beats_at);
    REQUIRE(beats_at < truths_at);
    REQUIRE(truths_at < take_at);
    REQUIRE(take_at < raw_at);

    const auto stim_at = user.find("The gate swings open");
    REQUIRE(stim_at != std::string::npos);
    REQUIRE(stim_at > take_at);

    const auto fact_at = user.find("The gate has opened");
    REQUIRE(fact_at != std::string::npos);
    REQUIRE(fact_at > raw_at);
    const auto hash_at = user.find('#', fact_at);
    REQUIRE(hash_at != std::string::npos);
    REQUIRE(fact_at < hash_at);

    REQUIRE(first.find("Seed description") == std::string::npos);
    REQUIRE(user.find("JSON schema") == std::string::npos);

    std::string second;
    memory.update_monologues(
        2, "Careful", "A bird lands on the wall.",
        [&](const std::string& prompt) {
            second = prompt;
            return std::string{kListen};
        },
        voice);

    const auto split2 = second.find(sentinel);
    REQUIRE(split2 != std::string::npos);
    REQUIRE(second.substr(0, split2) == system);
    const std::string user2 = second.substr(split2 + sentinel.size());
    const auto take2 = user2.find("What just happened");
    REQUIRE(take2 != std::string::npos);
    REQUIRE(user.substr(0, take_at) == user2.substr(0, take2));
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
