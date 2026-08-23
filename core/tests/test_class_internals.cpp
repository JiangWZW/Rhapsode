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

TEST_CASE("CharacterMemory monologue update writes knows and stream lines",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.seed_belief("The gate is shut", {"Gate"}, 0);
    memory.seed_belief("The gate has opened", {"Gate"}, 1,
                       CharacterMemory::kAuthoredSeedWeight, "perception");

    memory.update_monologues(1, "Careful", "The gate swings open.",
        [](const std::string&) {
            return std::string{
                R"({"appends":[{"stream_id":"self","text":"Finally — a way out."}],"ops":[],"knows":[{"fact":"I can finally leave.","entities":["Gate"],"weight":7,"relation":"evidence"}],"core_revision":null})"};
        });

    const std::string view = memory.render_thoughts({"Gate"});
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

TEST_CASE("CharacterMemory monologue prompt is bible, foci, journal, then take",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.seed_belief("The gate is shut", {"Gate"}, 0);
    memory.seed_belief("The gate has opened", {"Gate"}, 1,
                       CharacterMemory::kAuthoredSeedWeight, "perception");

    const char* kListen =
        R"({"appends":[],"ops":[],"knows":[],"core_revision":null})";
    const std::string voice = "  Voice: clipped and dry\n";
    const std::string sentinel = "<<<RHAPSODE_MONOLOGUE_USER>>>";
    const auto user_of = [&](const std::string& prompt) {
        const auto split = prompt.find(sentinel);
        REQUIRE(split != std::string::npos);
        return prompt.substr(split + sentinel.size());
    };

    std::string first;
    memory.update_monologues(
        1, "Careful", "Turn\nassistant: The gate swings open.",
        [&](const std::string& prompt) {
            first = prompt;
            return std::string{kListen};
        },
        voice);

    const std::string system = first.substr(0, first.find(sentinel));
    const std::string user = user_of(first);

    REQUIRE(system.find("\"appends\"") != std::string::npos);
    REQUIRE(system.find("On your mind") != std::string::npos);
    REQUIRE(system.find("You are Scout") == std::string::npos);
    REQUIRE(system.find("The gate swings open") == std::string::npos);

    const auto name_at = user.find("You are Scout");
    const auto bible_at = user.find("Who you are:");
    const auto mind_at = user.find("On your mind:");
    const auto journal_at = user.find("What you've been thinking:");
    const auto take_at = user.find("What just happened:");
    REQUIRE(name_at != std::string::npos);
    REQUIRE(bible_at != std::string::npos);
    REQUIRE(mind_at != std::string::npos);
    REQUIRE(journal_at != std::string::npos);
    REQUIRE(take_at != std::string::npos);
    REQUIRE(user.find("I keep the watch.") != std::string::npos);
    REQUIRE(user.find("- self: ambient self") != std::string::npos);
    REQUIRE(name_at < bible_at);
    REQUIRE(bible_at < mind_at);
    REQUIRE(mind_at < journal_at);
    REQUIRE(journal_at < take_at);

    REQUIRE(user.find("clipped and dry") == std::string::npos);
    REQUIRE(user.find("Through-lines you are already carrying") == std::string::npos);
    REQUIRE(user.find("Recent inner beats") == std::string::npos);
    REQUIRE(user.find("What you already hold as true") == std::string::npos);
    REQUIRE(user.find("What reached you this take") == std::string::npos);
    REQUIRE(user.find("The gate is shut") == std::string::npos);
    REQUIRE(first.find("Seed description") == std::string::npos);
    REQUIRE(user.find("JSON schema") == std::string::npos);

    const auto stim_at = user.find("The gate swings open");
    REQUIRE(stim_at != std::string::npos);
    REQUIRE(stim_at > take_at);
    REQUIRE(user.find("The gate has opened") == std::string::npos);

    std::string second;
    memory.update_monologues(
        2, "Careful",
        "Turn\nassistant: The gate swings open.\nuser: A bird lands on the wall.",
        [&](const std::string& prompt) {
            second = prompt;
            return std::string{kListen};
        },
        voice);

    REQUIRE(second.substr(0, second.find(sentinel)) == system);
    const std::string user2 = user_of(second);
    const auto take2 = user2.find("What just happened:");
    REQUIRE(take2 != std::string::npos);
    REQUIRE(user.substr(0, take_at) == user2.substr(0, take2));
    REQUIRE(user2.find("The gate swings open") != std::string::npos);
    REQUIRE(user2.find("A bird lands on the wall") != std::string::npos);
}

TEST_CASE("CharacterMemory monologue journal appends stay a user-text prefix",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    const std::string sentinel = "<<<RHAPSODE_MONOLOGUE_USER>>>";
    const auto through_journal = [&](const std::string& prompt) {
        const auto split = prompt.find(sentinel);
        REQUIRE(split != std::string::npos);
        const std::string user = prompt.substr(split + sentinel.size());
        const auto take_at = user.find("What just happened:");
        REQUIRE(take_at != std::string::npos);
        return user.substr(0, take_at);
    };

    std::string before_fork;
    memory.update_monologues(
        1, "Careful", "Take one.",
        [&](const std::string& prompt) {
            before_fork = prompt;
            return std::string{
                R"({"appends":[],"ops":[{"op":"fork","parent":"self","focus":"the open gate","opening":"If that latch gives, I run."}],"knows":[],"core_revision":null})"};
        });

    std::string after_fork;
    std::string child_id;
    for (const auto& stream : memory.streams()) {
        if (stream.id != "self" && stream.status == "active")
            child_id = stream.id;
    }
    REQUIRE_FALSE(child_id.empty());

    memory.update_monologues(
        2, "Careful", "Take two.",
        [&](const std::string& prompt) {
            after_fork = prompt;
            return std::string{
                "{\"appends\":[{\"stream_id\":\"self\",\"text\":\"Stay on the wall.\"}],"
                "\"ops\":[],\"knows\":[],\"core_revision\":null}"};
        });

    const std::string head_before_fork = through_journal(before_fork);
    const std::string head_after_fork = through_journal(after_fork);
    REQUIRE(head_before_fork.find("On your mind:") != std::string::npos);
    REQUIRE(head_after_fork.find(child_id) != std::string::npos);
    REQUIRE(head_before_fork != head_after_fork);

    std::string after_self_append;
    memory.update_monologues(
        3, "Careful", "Take three.",
        [&](const std::string& prompt) {
            after_self_append = prompt;
            return std::string{R"({"appends":[],"ops":[],"knows":[],"core_revision":null})"};
        });

    const std::string head_after_self = through_journal(after_self_append);
    REQUIRE(head_after_fork == head_after_self.substr(0, head_after_fork.size()));
    REQUIRE(head_after_self.find("[self] Stay on the wall.") != std::string::npos);
    const auto child_line = head_after_self.find("[" + child_id + "] If that latch gives");
    const auto self_line = head_after_self.find("[self] Stay on the wall.");
    REQUIRE(child_line != std::string::npos);
    REQUIRE(self_line != std::string::npos);
    REQUIRE(child_line < self_line);
}

TEST_CASE("CharacterMemory line seq persists and old saves backfill",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.update_monologues(
        1, "Careful", "Take.",
        [](const std::string&) {
            return std::string{
                R"({"appends":[{"stream_id":"self","text":"Keep still."}],"ops":[],"knows":[],"core_revision":null})"};
        });
    const auto saved = memory.to_json();
    REQUIRE(saved.contains("line_cnt"));
    REQUIRE(saved["line_cnt"].get<int>() >= 1);
    REQUIRE(saved["streams"][0]["lines"][0]["seq"].get<int>() == 1);

    const CharacterMemory loaded = CharacterMemory::from_json(saved);
    REQUIRE(loaded.streams().front().lines.front().seq == 1);
    REQUIRE(loaded.to_json()["line_cnt"].get<int>()
            == saved["line_cnt"].get<int>());

    json legacy = saved;
    legacy.erase("line_cnt");
    legacy.erase("next_line_id");
    legacy.erase("line_seq");
    for (auto& stream : legacy["streams"]) {
        for (auto& line : stream["lines"])
            line.erase("seq");
    }
    const CharacterMemory backfilled = CharacterMemory::from_json(legacy);
    REQUIRE(backfilled.streams().front().lines.front().seq == 1);
    REQUIRE(backfilled.to_json()["line_cnt"].get<int>() >= 1);
}

TEST_CASE("Objective journal appends take then seen and persists",
          "[character_memory][characterization]") {
    CharacterMemory memory("Scout");
    memory.ensure_bootstrap("I keep the watch.");
    memory.append_objective(1, "take", "The gate swings open.");
    memory.update_objective_journal(1, "I keep the watch.",
        [](const std::string& prompt) {
            REQUIRE(prompt.find("<<<RHAPSODE_OBSERVATION_USER>>>")
                    != std::string::npos);
            REQUIRE(prompt.find("[1 take]") != std::string::npos);
            REQUIRE(prompt.find("The gate swings open.") != std::string::npos);
            return std::string{
                R"({"lines":["The latch gave."],"facts":[{"fact":"The gate is open","entities":["Gate"]}]})"};
        });
    REQUIRE(memory.objective_journal().size() == 2);
    REQUIRE(memory.objective_journal()[1].type == "seen");
    REQUIRE(memory.objective_journal()[1].text == "The latch gave.");

    const CharacterMemory loaded =
        CharacterMemory::from_json(memory.to_json());
    REQUIRE(loaded.objective_journal().size() == 2);
    REQUIRE(loaded.objective_journal()[0].type == "take");

    memory.update_objective_journal(1, "I keep the watch.",
        [](const std::string&) {
            return std::string{R"({"lines":[],"facts":[]})"};
        });
    REQUIRE(memory.objective_journal().size() == 2);

    std::string monologue;
    memory.update_monologues(1, "Careful", "The latch gave.",
        [&](const std::string& prompt) {
            monologue = prompt;
            return std::string{
                R"({"appends":[],"ops":[],"knows":[],"core_revision":null})"};
        });
    const auto user = monologue.substr(
        monologue.find("<<<RHAPSODE_MONOLOGUE_USER>>>"));
    REQUIRE(user.find("The latch gave.") != std::string::npos);
    REQUIRE(user.find("The gate swings open.") == std::string::npos);
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
