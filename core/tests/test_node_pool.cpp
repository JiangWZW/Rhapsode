#include <catch2/catch_test_macros.hpp>
#include "rhapsode/node_pool.h"

using namespace rhapsode;

TEST_CASE("NodePool basic add/get/query", "[node_pool]") {
    NodePool pool;
    Node n;
    n.fact = "The barkeep owes money to the guild";
    n.type = "plot";
    n.state = NodeState::Foreshadowed;
    n.entities = {"barkeep"};
    n.known_by = {"player"};
    n.foreshadow_ctx = "The barkeep glances nervously at the door.";
    Node& added = pool.add(n);

    REQUIRE(added.id > 0);
    REQUIRE(pool.get(added.id) != nullptr);
    REQUIRE(pool.by_state(NodeState::Foreshadowed).size() == 1);
    REQUIRE(pool.by_entity("barkeep").size() == 1);
    REQUIRE(pool.by_known_by("player").size() == 1);
}

TEST_CASE("NodePool JSON round-trip", "[node_pool]") {
    NodePool pool;
    Node n1;
    n1.fact = "Aldric is in the tavern";
    n1.type = "location";
    n1.state = NodeState::Active;
    n1.entities = {"aldric", "tavern"};
    pool.add(n1);

    Node n2;
    n2.fact = "The knight is the missing prince";
    n2.type = "secret";
    n2.state = NodeState::Dormant;
    n2.entities = {"aldric"};
    pool.add(n2);

    auto j = pool.to_json();
    NodePool restored = NodePool::from_json(j);

    REQUIRE(restored.all_nodes().size() == 2);
    REQUIRE(restored.by_state(NodeState::Active).size() == 1);
    REQUIRE(restored.by_entity("aldric").size() == 2);
}
