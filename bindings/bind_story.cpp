#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/history.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_story(py::module_& m) {
    // -- Messages & History --

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
        .def("truncate",  &History::truncate, py::arg("new_size"))
        .def("clear",     &History::clear)
        .def("messages",  &History::messages, py::return_value_policy::reference_internal);

    // -- Scene --

    py::class_<Character>(m, "Character")
        .def(py::init<>())
        .def(py::init<std::string, std::string, bool>(),
            py::arg("name"), py::arg("description"), py::arg("is_player") = false)
        .def_readwrite("name",                   &Character::name)
        .def_readwrite("description",            &Character::description)
        .def_readwrite("dialogue_instructions",  &Character::dialogue_instructions)
        .def_readwrite("example_dialogue",       &Character::example_dialogue)
        .def_readwrite("role",                   &Character::role)
        .def_readwrite("is_player",              &Character::is_player)
        .def_readwrite("scene_ids",              &Character::scene_ids)
        .def_property_readonly("on_stage",       &Character::on_stage)
        .def("in_scene",                         &Character::in_scene, py::arg("scene_id"))
        .def("join_scene",                       &Character::join_scene, py::arg("scene_id"))
        .def("leave_scene",                      &Character::leave_scene, py::arg("scene_id"))
        .def_readwrite("dead",                   &Character::dead)
        .def_readwrite("created_at",             &Character::created_at)
        .def("__repr__", [](const Character& c) {
            std::string tag = c.dead ? "dead" : (c.on_stage() ? "on-stage" : "off-stage");
            return "Character(" + c.name + ", " + tag + ")";
        });

    py::class_<DeathCandidate>(m, "DeathCandidate")
        .def_readonly("character_name", &DeathCandidate::character_name)
        .def_readonly("evidence",       &DeathCandidate::evidence);

    py::class_<World>(m, "World")
        .def(py::init<>())
        .def_property("world_graph",
            [](World& w) -> WorldGraph& { return w.world_graph; },
            [](World& w, const WorldGraph& g) { w.world_graph = g; },
            py::return_value_policy::reference_internal)
        .def_readwrite("characters",         &World::characters)
        .def_readwrite("character_memories", &World::character_memories)
        .def("find_character", [](World& w, const std::string& name) -> Character* {
                return w.find_character(name);
             }, py::arg("name"), py::return_value_policy::reference_internal)
        .def("tool_query_graph",  &World::tool_query_graph,  py::arg("query"))
        .def("tool_query_mind",   &World::tool_query_mind,   py::arg("character"))
        .def("stage_fork", &World::stage_fork,
            py::arg("source_scene_id"), py::arg("driving_intention"), py::arg("cast"))
        .def("stage_conclude", &World::stage_conclude,
            py::arg("source_scene_id"), py::arg("reason"))
        .def("stage_merge", &World::stage_merge,
            py::arg("source_scene_id"), py::arg("into_scene_id"))
        .def("stage_exit", &World::stage_exit,
            py::arg("source_scene_id"), py::arg("cast"))
        .def("clear_pending_ops", &World::clear_pending_ops)
        .def("scan_death_candidates", &World::scan_death_candidates)
        .def("set_reflection_llm_callback", &World::set_reflection_llm_callback,
             py::arg("cb"))
        .def("set_memory", &World::set_memory, py::arg("mem"), py::keep_alive<1, 2>())
        .def("has_save",   &World::has_save,   py::arg("saves_dir"))
        .def("load_save",  &World::load_save,  py::arg("saves_dir"))
        .def("save",       &World::save,       py::arg("saves_dir"));

    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("title",              &Scene::title)
        .def_readwrite("system_prompt",      &Scene::system_prompt)
        // Roster/graph/minds now live on the shared World; expose them on Scene
        // as forwarding properties so existing Python code is unchanged.
        .def_property("characters",
            [](Scene& s) -> std::vector<Character>& { return s.world().characters; },
            [](Scene& s, const std::vector<Character>& v) { s.world().characters = v; },
            py::return_value_policy::reference_internal)
        .def_readwrite("history",            &Scene::history)
        .def_readwrite("dialogue",           &Scene::dialogue)
        .def_readwrite("scene_id",           &Scene::scene_id)
        .def_readwrite("turn_index",         &Scene::turn_index)
        .def_readwrite("driving_intention",  &Scene::driving_intention)
        .def_readwrite("charge",             &Scene::charge)
        .def_readwrite("last_advanced",      &Scene::last_advanced)
        .def_property("character_memories",
            [](Scene& s) -> std::unordered_map<std::string, CharacterMemory>& {
                return s.world().character_memories; },
            [](Scene& s, const std::unordered_map<std::string, CharacterMemory>& memories) {
                s.world().character_memories = memories; },
            py::return_value_policy::reference_internal)
        .def_readwrite("downsampler",        &Scene::downsampler)
        .def_property("world_graph",
            [](Scene& s) -> WorldGraph& { return s.world().world_graph; },
            [](Scene& s, const WorldGraph& g) { s.world().world_graph = g; },
            py::return_value_policy::reference_internal)
        .def("world", [](Scene& s) -> World& { return s.world(); },
             py::return_value_policy::reference_internal)
        .def("enter_character",  &Scene::enter_character, py::arg("ch"),
             py::return_value_policy::reference_internal)
        .def("find_on_stage", [](Scene& self, const std::string& name) -> Character* {
                return self.find_on_stage(name);
             }, py::arg("name"), py::return_value_policy::reference_internal)
        .def("exit_character",          &Scene::exit_character, py::arg("name"))
        .def("scan_death_candidates",   &Scene::scan_death_candidates)
        .def("set_memory",  &Scene::set_memory, py::arg("mem"),
             py::keep_alive<1, 2>())
        .def("has_save",    &Scene::has_save, py::arg("saves_dir"))
        .def("load_save",   &Scene::load_save, py::arg("saves_dir"))
        .def("save",        &Scene::save, py::arg("saves_dir"))
        .def("fork", &Scene::fork, py::arg("new_scene_id"), py::arg("cast"),
             "Fork a new storyline over the same World; cast names join the child.")
        .def("revert_turns", &Scene::revert_turns, py::arg("n"))
        .def("display_timeline", &Scene::display_timeline, py::arg("cap") = std::nullopt,
             "Chronological merge of history + dialogue for UI replay.")
        .def("tool_query_graph",  &Scene::tool_query_graph,  py::arg("query"))
        .def("tool_query_mind",   &Scene::tool_query_mind,   py::arg("character"))
        .def("tool_query_history",&Scene::tool_query_history, py::arg("query"))
        .def("delete_save", &Scene::delete_save, py::arg("saves_dir"))
        .def_static("load_json",  &Scene::load_json)
        .def("save_json",         &Scene::save_json)
        .def("to_json_str",   [](const Scene& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return Scene::from_json(nlohmann::json::parse(s));
        });

    py::class_<Story>(m, "Story")
        .def(py::init<>())
        .def_static("from_scene", &Story::from_scene, py::arg("root"))
        .def("world", [](Story& s) -> World& { return s.world(); },
            py::return_value_policy::reference_internal)
        .def("get_scene", [](Story& s, const std::string& id) { return s.get_scene(id); },
            py::arg("id"), py::return_value_policy::reference_internal)
        .def("scene_ids",   &Story::scene_ids)
        .def("scene_count", &Story::scene_count)
        .def_property("active_scene_id", &Story::active_scene_id, &Story::set_active_scene)
        .def("active_scene", &Story::active_scene,
            py::return_value_policy::reference_internal)
        .def("fork_scene", &Story::fork_scene,
            py::arg("parent_id"), py::arg("new_id"), py::arg("cast"),
            py::arg("driving_intention") = "",
            py::return_value_policy::reference_internal)
        .def("conclude_scene", &Story::conclude_scene, py::arg("id"), py::arg("reason"))
        .def("merge_scene", &Story::merge_scene, py::arg("from_id"), py::arg("into_id"))
        .def("apply_pending_ops", &Story::apply_pending_ops)
        .def("note_advanced", &Story::note_advanced, py::arg("scene_id"))
        .def_property_readonly("beat_clock", &Story::beat_clock)
        .def("tool_list_scenes", &Story::tool_list_scenes)
        .def("dispatch_tool", &Story::dispatch_tool,
            py::arg("scene_id"), py::arg("name"), py::arg("args_json"))
        .def("bind_runtime", &Story::bind_runtime, py::arg("loop"),
            py::keep_alive<1, 2>())
        .def("set_scheduler_callback", &Story::set_scheduler_callback, py::arg("cb"))
        .def("set_lifecycle_callback", &Story::set_lifecycle_callback, py::arg("cb"))
        .def("set_downsampler_callback", &Story::set_downsampler_callback, py::arg("cb"))
        .def("set_saves_dir", &Story::set_saves_dir, py::arg("dir"))
        .def("advance_scene", &Story::advance_scene, py::arg("player_input"),
            py::call_guard<py::gil_scoped_release>())
        .def("has_save",   &Story::has_save,   py::arg("saves_dir"))
        .def("load_save",  &Story::load_save,  py::arg("saves_dir"))
        .def("save",       &Story::save,       py::arg("saves_dir"))
        .def("delete_save", &Story::delete_save, py::arg("saves_dir"));
}
