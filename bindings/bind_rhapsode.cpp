#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "rhapsode/scene_message.h"
#include "rhapsode/history.h"
#include "rhapsode/character.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/node.h"
#include "rhapsode/node_pool.h"
#include "rhapsode/director.h"

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
        .def_readwrite("created_at",     &Node::created_at)
        .def_readwrite("resolved_at",    &Node::resolved_at)
        .def("__repr__", [](const Node& n) {
            return "Node(" + std::to_string(n.id) + ", "
                   + to_string(n.state) + ", \""
                   + n.fact.substr(0, 40) + "\")";
        });

    py::class_<NodePool>(m, "NodePool")
        .def(py::init<>())
        .def("add",        &NodePool::add,        py::return_value_policy::reference_internal)
        .def("get",        [](NodePool& self, std::uint64_t id) -> Node* { return self.get(id); },
                           py::return_value_policy::reference_internal)
        .def("remove",     &NodePool::remove)
        .def("size",       &NodePool::size)
        .def("by_state",   &NodePool::by_state,   py::return_value_policy::reference_internal)
        .def("by_entity",  &NodePool::by_entity,  py::return_value_policy::reference_internal)
        .def("by_known_by",&NodePool::by_known_by, py::return_value_policy::reference_internal)
        .def("wavefront",  &NodePool::wavefront,  py::return_value_policy::reference_internal)
        .def("all_nodes",  &NodePool::all_nodes)
        .def("to_json_str",   [](const NodePool& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return NodePool::from_json(nlohmann::json::parse(s));
        })
        .def("__len__", &NodePool::size);

    // ── Director ──

    py::class_<DirectorOutput>(m, "DirectorOutput")
        .def(py::init<>())
        .def_readwrite("context_blocks",  &DirectorOutput::context_blocks)
        .def_readwrite("newly_resolved",  &DirectorOutput::newly_resolved);

    py::class_<Director>(m, "Director")
        .def(py::init<NodePool&>(), py::arg("pool"))
        .def("set_llm_callback",        &Director::set_llm_callback)
        .def("set_retrieval_callback",   &Director::set_retrieval_callback)
        .def("tick",                     &Director::tick, py::arg("turn_index"), py::arg("scene_context"));

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
        .def("load_scene",                  &SceneLoop::load_scene)
        .def("submit_input",                &SceneLoop::submit_input)
        .def("state",                       &SceneLoop::state)
        .def("set_prompt_callback",         &SceneLoop::set_prompt_callback)
        .def("set_llm_callback",            &SceneLoop::set_llm_callback)
        .def("set_turn_complete_callback",  &SceneLoop::set_turn_complete_callback)
        .def("set_director",                &SceneLoop::set_director, py::arg("director"))
        .def("last_director_output",        &SceneLoop::last_director_output);
}
