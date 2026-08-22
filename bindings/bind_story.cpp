#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
#include "rhapsode/weaver.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_story(py::module_& m) {
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
            [](const SceneMessage& message) { return message.metadata.dump(); },
            [](SceneMessage& message, const std::string& value) {
                message.metadata = nlohmann::json::parse(value);
            })
        .def("__repr__", [](const SceneMessage& message) {
            nlohmann::json value;
            to_json(value, message);
            return "SceneMessage(" + value["role"].get<std::string>() + ": " +
                   value["content"].get<std::string>().substr(0, 40) + ")";
        });

    py::class_<Character>(m, "Character")
        .def(py::init<>())
        .def(py::init<std::string, std::string, bool>(),
             py::arg("name"), py::arg("description"), py::arg("is_player") = false)
        .def_readwrite("name", &Character::name)
        .def_readwrite("description", &Character::description)
        .def_readwrite("dialogue_instructions", &Character::dialogue_instructions)
        .def_readwrite("example_dialogue", &Character::example_dialogue)
        .def_readwrite("role", &Character::role)
        .def_readwrite("is_player", &Character::is_player)
        .def_property_readonly("scene_ids",
            [](const Character& character) { return character.scene_ids; })
        .def_property_readonly("on_stage", &Character::on_stage)
        .def("in_scene", &Character::in_scene, py::arg("scene_id"))
        .def_property_readonly("dead", [](const Character& character) {
            return character.dead;
        })
        .def_readwrite("created_at", &Character::created_at)
        .def("__repr__", [](const Character& character) {
            const std::string tag = character.dead
                ? "dead" : (character.on_stage() ? "on-stage" : "off-stage");
            return "Character(" + character.name + ", " + tag + ")";
        });

    py::class_<World>(m, "World")
        .def(py::init<>())
        .def_property_readonly("state_version", &World::state_version)
        .def_property_readonly("world_graph",
            [](const World& world) { return world.graph(); })
        .def_property_readonly("characters",
            [](const World& world) {
                return world.characters();
            })
        .def_property_readonly("character_memories",
            [](const World& world) {
                return world.character_memories();
            })
        .def("find_character",
            [](const World& world, const std::string& name)
                -> std::optional<Character> {
                const Character* character = world.find_character(name);
                return character ? std::optional<Character>(*character)
                                 : std::nullopt;
            }, py::arg("name"));

    py::class_<SceneData>(m, "SceneData")
        .def_readonly("scene_id", &SceneData::scene_id)
        .def_readonly("title", &SceneData::title)
        .def_readonly("system_prompt", &SceneData::system_prompt)
        .def_readonly("history", &SceneData::history)
        .def_readonly("dialogue", &SceneData::dialogue)
        .def_readonly("turn_index", &SceneData::turn_index)
        .def_readonly("driving_intention", &SceneData::driving_intention)
        .def_readonly("charge", &SceneData::charge)
        .def_readonly("last_advanced", &SceneData::last_advanced)
        .def_readonly("intention_owner", &SceneData::intention_owner)
        .def_readonly("intention_node_id", &SceneData::intention_node_id);

    py::class_<Story>(m, "Story")
        .def(py::init<>())
        .def_static("load_scenario", &Story::load_scenario, py::arg("path"))
        .def_static("from_scenario_json_str", [](const std::string& value,
                                                   const std::string& scene_id) {
            return Story::from_scenario_json(nlohmann::json::parse(value), scene_id);
        }, py::arg("value"), py::arg("scene_id") = "")
        .def("to_scenario_json_str", [](const Story& story, const std::string& scene_id) {
            return story.to_scenario_json(scene_id).dump(2);
        }, py::arg("scene_id"))
        .def("world", &Story::world_snapshot)
        .def("get_scene", [](const Story& story, const std::string& id)
                -> std::optional<SceneData> {
            const SceneData* scene = story.get_scene(id);
            return scene ? std::optional<SceneData>(*scene) : std::nullopt;
        }, py::arg("id"))
        .def("scene_ids", &Story::scene_ids)
        .def("scene_count", &Story::scene_count)
        .def_property("active_scene_id", &Story::active_scene_id,
                      &Story::set_active_scene)
        .def("active_scene", [](const Story& story)
                -> std::optional<SceneData> {
            const SceneData* scene = story.active_scene();
            return scene ? std::optional<SceneData>(*scene) : std::nullopt;
        })
        .def("fork_scene", [](Story& story, const std::string& parent_id,
                              const std::string& new_id,
                              const std::vector<std::string>& cast,
                              const std::string& driving_intention)
                -> std::optional<SceneData> {
            SceneData* scene = story.fork_scene(
                parent_id, new_id, cast, driving_intention);
            return scene ? std::optional<SceneData>(*scene) : std::nullopt;
        }, py::arg("parent_id"), py::arg("new_id"), py::arg("cast"),
           py::arg("driving_intention") = "")
        .def("conclude_scene", &Story::conclude_scene,
             py::arg("id"), py::arg("reason"))
        .def("merge_scene", &Story::merge_scene,
             py::arg("from_id"), py::arg("into_id"))
        .def("note_advanced", &Story::note_advanced, py::arg("scene_id"))
        .def_property_readonly("turn_clock", &Story::turn_clock)
        .def_property_readonly("beat_clock", &Story::beat_clock)
        .def("tool_list_scenes", &Story::tool_list_scenes)
        .def("dispatch_tool", &Story::dispatch_tool,
             py::arg("scene_id"), py::arg("name"), py::arg("args_json"))
        .def("display_timeline", &Story::display_timeline,
             py::arg("scene_id"), py::arg("cap") = std::nullopt)
        .def("render_transcript", &Story::render_transcript)
        .def("set_llm_callback", &Story::set_llm_callback, py::arg("cb"))
        .def("set_narrator_llm_callback", &Story::set_narrator_llm_callback,
             py::arg("cb"))
        .def("set_weaver_llm_callback", &Story::set_weaver_llm_callback,
             py::arg("cb"))
        .def("set_weaver_interval", &Story::set_weaver_interval,
             py::arg("turns"))
        .def("weave_scene", &Story::weave_scene, py::arg("scene_id"))
        .def("set_history_window", &Story::set_history_window,
             py::arg("normal") = 3, py::arg("resume") = 10)
        .def("set_resuming", &Story::set_resuming, py::arg("value"))
        .def("set_scheduler_callback", &Story::set_scheduler_callback, py::arg("cb"))
        .def("set_lifecycle_callback", &Story::set_lifecycle_callback, py::arg("cb"))
        .def("set_downsampler_callback", &Story::set_downsampler_callback, py::arg("cb"))
        .def("set_reflection_llm_callback", &Story::set_reflection_llm_callback,
             py::arg("cb"))
        .def("set_memory", &Story::set_memory, py::arg("memory"))
        .def("set_saves_dir", &Story::set_saves_dir, py::arg("dir"))
        .def("advance_player", &Story::advance_player, py::arg("player_input"),
             py::call_guard<py::gil_scoped_release>())
        .def("complete_turn", &Story::complete_turn,
             py::call_guard<py::gil_scoped_release>())
        .def("revert_active_turns", &Story::revert_active_turns, py::arg("count"))
        .def("has_save", &Story::has_save, py::arg("saves_dir"))
        .def("load_save", &Story::load_save, py::arg("saves_dir"))
        .def("save", &Story::save, py::arg("saves_dir"))
        .def("delete_save", &Story::delete_save, py::arg("saves_dir"));
}
