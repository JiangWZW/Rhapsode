#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "rhapsode/scene_message.h"
#include "rhapsode/history.h"
#include "rhapsode/character.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop.h"

namespace py = pybind11;
using namespace rhapsode;

PYBIND11_MODULE(_core, m) {
    m.doc() = "Rhapsode C++ core bindings";

    py::enum_<Role>(m, "Role")
        .value("System", Role::System)
        .value("User", Role::User)
        .value("Assistant", Role::Assistant);

    py::class_<SceneMessage>(m, "SceneMessage")
        .def(py::init<>())
        .def_readwrite("role", &SceneMessage::role)
        .def_readwrite("content", &SceneMessage::content)
        .def_readwrite("timestamp", &SceneMessage::timestamp)
        .def_property("metadata",
            [](const SceneMessage& self) { return self.metadata.dump(); },
            [](SceneMessage& self, const std::string& s) {
                self.metadata = nlohmann::json::parse(s);
            })
        .def("__repr__", [](const SceneMessage& m) {
            nlohmann::json j;
            to_json(j, m);
            return "SceneMessage(" + j["role"].get<std::string>() + ": " + j["content"].get<std::string>().substr(0, 40) + ")";
        });

    py::class_<History>(m, "History")
        .def(py::init<>())
        .def("append", &History::append)
        .def("snapshot", &History::snapshot, py::arg("n") = py::none())
        .def("size", &History::size)
        .def("clear", &History::clear)
        .def("messages", &History::messages, py::return_value_policy::reference_internal);

    py::class_<Character>(m, "Character")
        .def(py::init<>())
        .def(py::init<std::string, std::string, bool>(),
            py::arg("name"), py::arg("description"), py::arg("is_player") = false)
        .def_readwrite("name", &Character::name)
        .def_readwrite("description", &Character::description)
        .def_readwrite("is_player", &Character::is_player)
        .def("__repr__", [](const Character& c) {
            return "Character(" + c.name + ")";
        });

    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("title", &Scene::title)
        .def_readwrite("system_prompt", &Scene::system_prompt)
        .def_readwrite("characters", &Scene::characters)
        .def_readwrite("history", &Scene::history)
        .def_static("load_json", &Scene::load_json)
        .def("save_json", &Scene::save_json)
        .def("to_json_str", [](const Scene& self) {
            return self.to_json().dump(2);
        })
        .def_static("from_json_str", [](const std::string& s) {
            return Scene::from_json(nlohmann::json::parse(s));
        });

    py::enum_<LoopState>(m, "LoopState")
        .value("Idle", LoopState::Idle)
        .value("WaitingForInput", LoopState::WaitingForInput)
        .value("ProcessingInput", LoopState::ProcessingInput)
        .value("BuildingPrompt", LoopState::BuildingPrompt)
        .value("RunningLLM", LoopState::RunningLLM)
        .value("AppendingResult", LoopState::AppendingResult);

    py::class_<SceneLoop>(m, "SceneLoop")
        .def(py::init<>())
        .def("load_scene", &SceneLoop::load_scene)
        .def("submit_input", &SceneLoop::submit_input)
        .def("state", &SceneLoop::state)
        .def("set_prompt_callback", &SceneLoop::set_prompt_callback)
        .def("set_llm_callback", &SceneLoop::set_llm_callback)
        .def("set_turn_complete_callback", &SceneLoop::set_turn_complete_callback);
}
