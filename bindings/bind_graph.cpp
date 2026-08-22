#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/annotator.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/graph_plan.h"
#include "rhapsode/node.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

namespace py = pybind11;
using namespace rhapsode;

namespace {

// Python compatibility object. Core turn code uses apply_graph_plan directly;
// only this boundary object retains a standalone graph supplied by Python.
struct BoundDirector {
    explicit BoundDirector(WorldGraph& graph) : graph(&graph) {}

    GraphPlanResult apply_planned_turn(
        int turn_index, const std::string& json_text) {
        return apply_graph_plan(
            *graph, turn_index, nlohmann::json::parse(json_text));
    }

    WorldGraph* graph;
};

}  // namespace

void bind_graph(py::module_& m) {
    // -- Node System --

    py::enum_<NodeState>(m, "NodeState")
        .value("Dormant",      NodeState::Dormant)
        .value("Foreshadowed", NodeState::Foreshadowed)
        .value("Active",       NodeState::Active);

    py::class_<Node>(m, "Node")
        .def(py::init<>())
        .def_readwrite("id",             &Node::id)
        .def_readwrite("fact",           &Node::fact)
        .def_readwrite("type",           &Node::type)
        .def_readwrite("state",          &Node::state)
        .def_readwrite("foreshadow_ctx", &Node::foreshadow_ctx)
        .def_readwrite("active_ctx",     &Node::active_ctx)
        .def_readwrite("entities",       &Node::entities)
        .def_readwrite("trigger",        &Node::trigger)
        .def_readwrite("arc_position",   &Node::arc_position)
        .def_readwrite("related_to",     &Node::related_to)
        .def_readwrite("created_at",     &Node::created_at)
        .def_readwrite("valid_until",    &Node::valid_until)
        .def_readwrite("weight",         &Node::weight)
        .def("__repr__", [](const Node& n) {
            return "Node(" + std::to_string(n.id) + ", "
                   + to_string(n.state) + ", \""
                   + n.fact.substr(0, 40) + "\")";
        });

    py::class_<EdgeInfo>(m, "EdgeInfo")
        .def_readonly("from_id", &EdgeInfo::from_id)
        .def_readonly("to_id",   &EdgeInfo::to_id)
        .def_readonly("data",    &EdgeInfo::data);

    py::class_<EdgeData>(m, "EdgeData")
        .def_readonly("weight",     &EdgeData::weight)
        .def_readonly("created_at", &EdgeData::created_at)
        .def_readonly("active",     &EdgeData::active)
        .def_readonly("kind",       &EdgeData::kind);

    py::class_<WorldGraph>(m, "WorldGraph")
        .def(py::init<>())
        .def("add_node",   &WorldGraph::add_node, py::return_value_policy::reference_internal)
        .def("get_node",   [](WorldGraph& self, std::uint64_t id) -> Node* {
                                return self.get_node(id);
                            }, py::return_value_policy::reference_internal)
        .def("has_node",   &WorldGraph::has_node)
        .def("set_valid_until", &WorldGraph::set_valid_until)
        .def("add_relation", &WorldGraph::add_relation,
             py::arg("from_id"), py::arg("to_id"),
             py::arg("weight") = 1.0f,
             py::arg("created_at") = 0,
             py::arg("kind") = "")
        .def("set_edge_active", &WorldGraph::set_edge_active,
             py::arg("from_id"), py::arg("to_id"), py::arg("active"))
        .def("set_edge_weight", &WorldGraph::set_edge_weight,
             py::arg("from_id"), py::arg("to_id"), py::arg("weight"))
        .def("all_edges", &WorldGraph::all_edges)
        .def("neighbors", &WorldGraph::neighbors, py::arg("node_id"))
        .def("neighbors_within", &WorldGraph::neighbors_within,
             py::arg("source_id"), py::arg("max_hops"), py::arg("active_only") = true)
        .def("thread_containing", &WorldGraph::thread_containing, py::arg("seed_id"))
        .def("all_threads", &WorldGraph::all_threads)
        .def("entity_groups", &WorldGraph::entity_groups)
        .def("size", &WorldGraph::size)
        .def("all_nodes", [](const WorldGraph& self) { return self.all_nodes(false); })
        .def("all_nodes_including_expired", [](const WorldGraph& self) {
            return self.all_nodes(true);
        })
        .def("to_json_str", [](const WorldGraph& self) { return self.to_json().dump(2); })
        .def("to_dot", &WorldGraph::to_dot)
        .def_static("from_json_str", [](const std::string& s) {
            return WorldGraph::from_json(nlohmann::json::parse(s));
        })
        .def("__len__", &WorldGraph::size);

    // -- Character Memory --

    py::class_<CharacterMemory>(m, "CharacterMemory")
        .def(py::init<std::string>(), py::arg("name"))
        .def("seed_belief",       &CharacterMemory::seed_belief,
             py::arg("fact"), py::arg("entities"), py::arg("created_at"),
             py::arg("weight") = CharacterMemory::kAuthoredSeedWeight,
             py::arg("type") = "belief")
        .def("link_tension",      &CharacterMemory::link_tension,
             py::arg("a_id"), py::arg("b_id"), py::arg("turn"))
        .def("view_of",           &CharacterMemory::view_of, py::arg("subjects"))
        .def("route_fact",        &CharacterMemory::route_fact,
             py::arg("fact"), py::arg("entities"), py::arg("turn"))
        .def("ensure_bootstrap",  &CharacterMemory::ensure_bootstrap,
             py::arg("core_text_if_empty"))
        .def("update_monologues", &CharacterMemory::update_monologues,
             py::arg("turn"), py::arg("description"), py::arg("beat_stimulus"),
             py::arg("callback"))
        .def("render_thoughts",   &CharacterMemory::render_thoughts,
             py::arg("subjects") = std::vector<std::string>{})
        .def("render_mind_query", [](const CharacterMemory& memory,
                                     std::size_t max_belief_chars,
                                     std::size_t max_line_chars) {
                return memory.render_mind_query(max_belief_chars, max_line_chars)
                    .dump();
             },
             py::arg("max_belief_chars") = 1200,
             py::arg("max_line_chars") = 400)
        .def_property_readonly("beliefs", [](const CharacterMemory& memory) {
             return memory.beliefs();
        })
        .def_property_readonly("core_text", [](const CharacterMemory& memory) {
             return memory.core().text;
        })
        .def_property_readonly("active_stream_count",
             &CharacterMemory::active_stream_count)
        .def("to_json_str", [](const CharacterMemory& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return CharacterMemory::from_json(nlohmann::json::parse(s));
        })
        .def_property_readonly("name", &CharacterMemory::name)
        .def("__repr__", [](const CharacterMemory& self) {
            return "CharacterMemory(" + self.name() + ")";
        });

    // -- Director --

    py::class_<Rejection>(m, "Rejection")
        .def(py::init<>())
        .def_readwrite("fact",   &Rejection::fact)
        .def_readwrite("reason", &Rejection::reason);

    py::class_<GraphPlanResult>(m, "DirectorOutput")
        .def(py::init<>())
        .def_readwrite("context_blocks",  &GraphPlanResult::context_blocks)
        .def_readwrite("newly_expired",   &GraphPlanResult::newly_expired)
        .def_readwrite("new_nodes",       &GraphPlanResult::new_nodes)
        .def_readwrite("rejections",      &GraphPlanResult::rejections);

    py::class_<BoundDirector>(m, "Director")
        .def(py::init<WorldGraph&>(), py::arg("graph"), py::keep_alive<1, 2>())
        .def("apply_planned_turn", &BoundDirector::apply_planned_turn,
             py::arg("turn_index"), py::arg("json_txt"));

    // -- Annotator --

    py::class_<EntitySpan>(m, "EntitySpan")
        .def_readonly("start",    &EntitySpan::start)
        .def_readonly("end_",     &EntitySpan::end)
        .def_readonly("text",     &EntitySpan::text)
        .def_readonly("category", &EntitySpan::category);

    py::class_<Annotator>(m, "Annotator")
        .def(py::init<const World&>(), py::arg("world"), py::keep_alive<1, 2>())
        .def("set_ner_callback", &Annotator::set_ner_callback)
        .def("annotate", &Annotator::annotate, py::arg("text"));
}
