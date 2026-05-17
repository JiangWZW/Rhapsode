#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "rhapsode/scene_message.h"
#include "rhapsode/history.h"
#include "rhapsode/character.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/node.h"
#include "rhapsode/world_graph.h"
#include "rhapsode/director.h"
#include "rhapsode/memory_system.h"

namespace py = pybind11;
using namespace rhapsode;

PYBIND11_MODULE(_core, m) {
    m.doc() = "Rhapsode C++ core bindings";

    // ── Messages & History ──

    py::enum_<Role>(m, "Role")
        .value("System", Role::System)
        .value("User", Role::User)
        .value("Assistant", Role::Assistant);

    py::class_<SceneMessage>(m, "SceneMessage")
        .def(py::init<>())
        .def_readwrite("role",      &SceneMessage::role)
        .def_readwrite("content",   &SceneMessage::content)
        .def_readwrite("timestamp", &SceneMessage::timestamp)
        .def_property("metadata",
            [](const SceneMessage& self) { return self.metadata.dump(); },
            [](SceneMessage& self, const std::string& s) {
                self.metadata = nlohmann::json::parse(s);
            })
        .def("__repr__", [](const SceneMessage& msg) {
            nlohmann::json j;
            to_json(j, msg);
            auto role    = j["role"].get<std::string>();
            auto preview = j["content"].get<std::string>().substr(0, 40);
            return "SceneMessage(" + role + ": " + preview + ")";
        });

    py::class_<History>(m, "History")
        .def(py::init<>())
        .def("append",   &History::append)
        .def("snapshot",  &History::snapshot, py::arg("n") = py::none())
        .def("size",      &History::size)
        .def("clear",     &History::clear)
        .def("messages",  &History::messages, py::return_value_policy::reference_internal);

    // ── Scene ──

    py::class_<Character>(m, "Character")
        .def(py::init<>())
        .def(py::init<std::string, std::string, bool>(),
            py::arg("name"), py::arg("description"), py::arg("is_player") = false)
        .def_readwrite("name",        &Character::name)
        .def_readwrite("description", &Character::description)
        .def_readwrite("is_player",   &Character::is_player)
        .def("__repr__", [](const Character& c) {
            return "Character(" + c.name + ")";
        });

    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("title",         &Scene::title)
        .def_readwrite("system_prompt", &Scene::system_prompt)
        .def_readwrite("characters",    &Scene::characters)
        .def_readwrite("history",       &Scene::history)
        .def_readwrite("scene_id",      &Scene::scene_id)
        .def_readwrite("turn_index",    &Scene::turn_index)
        .def_property("world_graph",
            [](Scene& s) -> WorldGraph& { return s.world_graph; },
            [](Scene& s, const WorldGraph& g) { s.world_graph = g; },
            py::return_value_policy::reference_internal)
        .def("set_memory",  &Scene::set_memory, py::arg("mem"),
             py::keep_alive<1, 2>())
        .def("has_save",    &Scene::has_save, py::arg("saves_dir"))
        .def("load_save",   &Scene::load_save, py::arg("saves_dir"))
        .def("save",        &Scene::save, py::arg("saves_dir"))
        .def("delete_save", &Scene::delete_save, py::arg("saves_dir"))
        .def_static("load_json",  &Scene::load_json)
        .def("save_json",         &Scene::save_json)
        .def("to_json_str",   [](const Scene& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return Scene::from_json(nlohmann::json::parse(s));
        });

    // ── Node System ──

    py::enum_<NodeState>(m, "NodeState")
        .value("Dormant",      NodeState::Dormant)
        .value("Foreshadowed", NodeState::Foreshadowed)
        .value("Active",       NodeState::Active)
        .value("Resolved",     NodeState::Resolved);

    py::class_<Node>(m, "Node")
        .def(py::init<>())
        .def_readwrite("id",             &Node::id)
        .def_readwrite("fact",           &Node::fact)
        .def_readwrite("type",           &Node::type)
        .def_readwrite("state",          &Node::state)
        .def_readwrite("foreshadow_ctx", &Node::foreshadow_ctx)
        .def_readwrite("active_ctx",     &Node::active_ctx)
        .def_readwrite("entities",       &Node::entities)
        .def_readwrite("known_by",       &Node::known_by)
        .def_readwrite("related_to",     &Node::related_to)
        .def_readwrite("created_at",     &Node::created_at)
        .def_readwrite("resolved_at",    &Node::resolved_at)
        .def("__repr__", [](const Node& n) {
            return "Node(" + std::to_string(n.id) + ", "
                   + to_string(n.state) + ", \""
                   + n.fact.substr(0, 40) + "\")";
        });

    py::enum_<RelationKind>(m, "RelationKind")
        .value("Related", RelationKind::Related)
        .value("Supersedes", RelationKind::Supersedes)
        .value("Contradicts", RelationKind::Contradicts)
        .value("CausedBy", RelationKind::CausedBy);

    py::class_<WorldGraph>(m, "WorldGraph")
        .def(py::init<>())
        .def("add_node",   &WorldGraph::add_node, py::return_value_policy::reference_internal)
        .def("get_node",   [](WorldGraph& self, std::uint64_t id) -> Node* { return self.get_node(id); },
                           py::return_value_policy::reference_internal)
        .def("has_node",   &WorldGraph::has_node)
        .def("mark_resolved", &WorldGraph::mark_resolved)
        .def("add_relation", &WorldGraph::add_relation,
             py::arg("from_id"), py::arg("to_id"),
             py::arg("kind") = RelationKind::Related,
             py::arg("confidence") = 1.0f,
             py::arg("created_at") = 0)
        .def("neighbors", [](const WorldGraph& self, std::uint64_t node_id) {
            return self.neighbors(node_id, std::nullopt);
        }, py::arg("node_id"))
        .def("neighbors_by_kind", [](const WorldGraph& self, std::uint64_t node_id, RelationKind kind) {
            return self.neighbors(node_id, kind);
        }, py::arg("node_id"), py::arg("kind"))
        .def("neighbors_within", [](const WorldGraph& self, std::uint64_t source_id, int max_hops) {
            return self.neighbors_within(source_id, max_hops, std::nullopt, true);
        }, py::arg("source_id"), py::arg("max_hops"))
        .def("neighbors_within_by_kind", [](const WorldGraph& self,
                                             std::uint64_t source_id,
                                             int max_hops,
                                             RelationKind kind,
                                             bool active_only) {
            return self.neighbors_within(source_id, max_hops, kind, active_only);
        }, py::arg("source_id"), py::arg("max_hops"), py::arg("kind"), py::arg("active_only") = true)
        .def("size", &WorldGraph::size)
        .def("all_nodes", [](const WorldGraph& self) { return self.all_nodes(false); })
        .def("all_nodes_including_resolved", [](const WorldGraph& self) { return self.all_nodes(true); })
        .def("to_json_str", [](const WorldGraph& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return WorldGraph::from_json(nlohmann::json::parse(s));
        })
        .def("__len__", &WorldGraph::size);

    // ── Director ──

    py::class_<DirectorOutput>(m, "DirectorOutput")
        .def(py::init<>())
        .def_readwrite("context_blocks",  &DirectorOutput::context_blocks)
        .def_readwrite("newly_resolved",  &DirectorOutput::newly_resolved)
        .def_readwrite("new_nodes",       &DirectorOutput::new_nodes);

    py::class_<Director>(m, "Director")
        .def(py::init<WorldGraph&>(), py::arg("graph"))
        .def("set_llm_callback", &Director::set_llm_callback)
        .def("tick", &Director::tick, py::arg("turn_index"), py::arg("scene_context"))
        .def("focus_payload_json", &Director::focus_payload_json, py::arg("turn_index"),
             py::arg("scene_context"))
        .def("apply_planned_turn",
             [](Director& self, int turn_idx, const std::string& json_txt) -> DirectorOutput {
                 return self.apply_planned_turn(turn_idx, nlohmann::json::parse(json_txt));
             },
             py::arg("turn_index"), py::arg("json_txt"));

    // ── Scene Loop ──

    py::enum_<LoopState>(m, "LoopState")
        .value("Idle",             LoopState::Idle)
        .value("WaitingForInput",  LoopState::WaitingForInput)
        .value("ProcessingInput",  LoopState::ProcessingInput)
        .value("BuildingPrompt",   LoopState::BuildingPrompt)
        .value("RunningLLM",       LoopState::RunningLLM)
        .value("AppendingResult",  LoopState::AppendingResult);

    py::class_<SceneLoop>(m, "SceneLoop")
        .def(py::init<>())
        .def("load_scene",                   &SceneLoop::load_scene)
        .def("submit_input",                 &SceneLoop::submit_input)
        .def("state",                        &SceneLoop::state)
        .def("set_prompt_callback",          &SceneLoop::set_prompt_callback)
        .def("set_llm_callback",             &SceneLoop::set_llm_callback)
        .def("set_turn_complete_callback",   &SceneLoop::set_turn_complete_callback)
        .def("set_character_synth_callback", &SceneLoop::set_character_synth_callback)
        .def("take_last_turn_outputs",       &SceneLoop::take_last_turn_outputs)
        .def("set_director",                 &SceneLoop::set_director, py::arg("director"))
        .def("last_director_output",         &SceneLoop::last_director_output)
        .def("set_history_window",           &SceneLoop::set_history_window,
             py::arg("normal") = 3, py::arg("resume") = 10)
        .def("set_resuming",                 &SceneLoop::set_resuming, py::arg("v"));

    // ── Memory System ──

    py::class_<MemorySystem>(m, "MemorySystem")
        .def(py::init<const std::string&>(), py::arg("scene_id"))
        .def("set_embed_callback",       &MemorySystem::set_embed_callback)
        .def("set_lemmatize_callback",   &MemorySystem::set_lemmatize_callback)
        .def("set_store_callback",       &MemorySystem::set_store_callback)
        .def("set_query_callback",       &MemorySystem::set_query_callback)
        .def("set_update_meta_callback", &MemorySystem::set_update_meta_callback)
        .def("set_get_by_meta_callback", &MemorySystem::set_get_by_meta_callback)
        .def("set_local_llm_callback",   &MemorySystem::set_local_llm_callback)
        .def("store_fact",
             [](MemorySystem& self,
                const std::string& fact,
                const std::string& state,
                const std::string& type,
                const std::vector<std::string>& known_by,
                const std::vector<std::string>& entities,
                int turn) {
                    return self.store_fact(fact, state, type, known_by, entities, {}, turn);
             },
             py::arg("fact"), py::arg("state"), py::arg("type"),
             py::arg("known_by"), py::arg("entities"), py::arg("turn"))
        .def("store_fact_with_links",
             py::overload_cast<const std::string&, const std::string&, const std::string&,
                               const std::vector<std::string>&, const std::vector<std::string>&,
                               const std::vector<std::uint64_t>&, int>(&MemorySystem::store_fact),
             py::arg("fact"), py::arg("state"), py::arg("type"),
             py::arg("known_by"), py::arg("entities"), py::arg("related_to"), py::arg("turn"))
        .def("retrieve",               &MemorySystem::retrieve,
             py::arg("query"), py::arg("top_k") = 8)
        .def("retrieve_for_injection", &MemorySystem::retrieve_for_injection,
             py::arg("scene_context"), py::arg("max_results") = 8)
        .def("process_new_nodes",      &MemorySystem::process_new_nodes,
             py::arg("nodes"), py::arg("turn"))
        .def("sync_resolved",          &MemorySystem::sync_resolved,
             py::arg("resolved_nodes"), py::arg("turn"))
        .def("get_next_id",            &MemorySystem::get_next_id)
        .def("set_next_id",            &MemorySystem::set_next_id, py::arg("id"));
}
