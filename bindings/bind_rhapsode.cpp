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
#include "rhapsode/character_memory.h"
#include "rhapsode/annotator.h"
#include "rhapsode/validator.h"
#include "rhapsode/weaver.h"
#include "rhapsode/text_downsampler.h"

namespace py = pybind11;
using namespace rhapsode;

PYBIND11_MODULE(_core, m) {
    m.doc() = "Rhapsode C++ core bindings";

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
        .def_readwrite("on_stage",               &Character::on_stage)
        .def_readwrite("dead",                   &Character::dead)
        .def_readwrite("created_at",             &Character::created_at)
        .def("__repr__", [](const Character& c) {
            std::string tag = c.dead ? "dead" : (c.on_stage ? "on-stage" : "off-stage");
            return "Character(" + c.name + ", " + tag + ")";
        });

    py::class_<DeathCandidate>(m, "DeathCandidate")
        .def_readonly("character_name", &DeathCandidate::character_name)
        .def_readonly("evidence",       &DeathCandidate::evidence);

    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def_readwrite("title",              &Scene::title)
        .def_readwrite("system_prompt",      &Scene::system_prompt)
        .def_readwrite("characters",         &Scene::characters)
        .def_readwrite("history",            &Scene::history)
        .def_readwrite("scene_id",           &Scene::scene_id)
        .def_readwrite("turn_index",         &Scene::turn_index)
        .def_readwrite("character_memories", &Scene::character_memories)
        .def_readwrite("downsampler",        &Scene::downsampler)
        .def_property("world_graph",
            [](Scene& s) -> WorldGraph& { return s.world_graph; },
            [](Scene& s, const WorldGraph& g) { s.world_graph = g; },
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
        .def("revert_turns", &Scene::revert_turns, py::arg("n"))
        .def("delete_save", &Scene::delete_save, py::arg("saves_dir"))
        .def_static("load_json",  &Scene::load_json)
        .def("save_json",         &Scene::save_json)
        .def("to_json_str",   [](const Scene& self) { return self.to_json().dump(2); })
        .def_static("from_json_str", [](const std::string& s) {
            return Scene::from_json(nlohmann::json::parse(s));
        });

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
        .def("get_node",   [](WorldGraph& self, std::uint64_t id) -> Node* { return self.get_node(id); },
                           py::return_value_policy::reference_internal)
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
        .def("all_nodes_including_expired", [](const WorldGraph& self) { return self.all_nodes(true); })
        .def("to_json_str", [](const WorldGraph& self) { return self.to_json().dump(2); })
        .def("to_dot", &WorldGraph::to_dot)
        .def_static("from_json_str", [](const std::string& s) {
            return WorldGraph::from_json(nlohmann::json::parse(s));
        })
        .def("__len__", &WorldGraph::size);

    // -- Character Memory --

    py::class_<CharacterMemory::MemoryNode>(m, "MemoryNode")
        .def(py::init<>())
        .def_readwrite("id",            &CharacterMemory::MemoryNode::id)
        .def_readwrite("type",          &CharacterMemory::MemoryNode::type)
        .def_readwrite("content",       &CharacterMemory::MemoryNode::content)
        .def_readwrite("created_at",    &CharacterMemory::MemoryNode::created_at)
        .def_readwrite("weight",        &CharacterMemory::MemoryNode::weight)
        .def_readwrite("depth",         &CharacterMemory::MemoryNode::depth)
        .def_readwrite("last_accessed", &CharacterMemory::MemoryNode::last_accessed)
        .def("__repr__", [](const CharacterMemory::MemoryNode& n) {
            return "MemoryNode(" + std::to_string(n.id) + ", \""
                   + n.content.substr(0, 40) + "\")";
        });

    py::class_<CharacterMemory>(m, "CharacterMemory")
        .def(py::init<std::string>(), py::arg("name"))
        .def("set_embed_callback",           &CharacterMemory::set_embed_callback)
        .def("set_store_callback",           &CharacterMemory::set_store_callback)
        .def("set_query_callback",           &CharacterMemory::set_query_callback)
        .def("set_reflection_llm_callback",  &CharacterMemory::set_reflection_llm_callback)
        .def("speak",             &CharacterMemory::speak,
             py::arg("scene_context"), py::arg("turn"))
        .def("observe",           &CharacterMemory::observe,
             py::arg("scene_context"), py::arg("turn"))
        .def("reflect",           &CharacterMemory::reflect)
        .def("needs_reflection",  &CharacterMemory::needs_reflection)
        .def("retrieve",          &CharacterMemory::retrieve,
             py::arg("query"), py::arg("top_k") = 5)
        .def("briefing",          &CharacterMemory::briefing,
             py::arg("query"), py::arg("top_k") = 5)
        .def("update_self_state", &CharacterMemory::update_self_state, py::arg("turn"))
        .def("set_self_state",    &CharacterMemory::set_self_state, py::arg("s"))
        .def_property_readonly("self_state", &CharacterMemory::self_state)
        .def("score_importance",  &CharacterMemory::score_importance,
             py::arg("description"))
        .def("seed_from_graph",   &CharacterMemory::seed_from_graph,
             py::arg("fact"), py::arg("created_at"))
        .def("seed_belief",       &CharacterMemory::seed_belief,
             py::arg("fact"), py::arg("entities"), py::arg("created_at"))
        .def("view_of",           &CharacterMemory::view_of, py::arg("subjects"))
        .def("route_fact",        &CharacterMemory::route_fact,
             py::arg("fact"), py::arg("entities"), py::arg("turn"))
        .def("reflect_perceptions", &CharacterMemory::reflect_perceptions, py::arg("turn"))
        .def_property_readonly("beliefs", &CharacterMemory::beliefs,
             py::return_value_policy::reference_internal)
        .def("sync_to_chroma",    &CharacterMemory::sync_to_chroma)
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

    py::class_<DirectorOutput>(m, "DirectorOutput")
        .def(py::init<>())
        .def_readwrite("context_blocks",  &DirectorOutput::context_blocks)
        .def_readwrite("newly_expired",   &DirectorOutput::newly_expired)
        .def_readwrite("new_nodes",       &DirectorOutput::new_nodes)
        .def_readwrite("rejections",      &DirectorOutput::rejections);

    py::class_<Director>(m, "Director")
        .def(py::init<WorldGraph&>(), py::arg("graph"))
        .def("set_llm_callback", &Director::set_llm_callback)
        .def("set_validator",    &Director::set_validator, py::arg("validator"))
        .def("tick", &Director::tick, py::arg("turn_index"), py::arg("scene_context"))
        .def("focus_payload_json", &Director::focus_payload_json, py::arg("turn_index"),
             py::arg("scene_context"))
        .def("focus_payload_text", &Director::focus_payload_text, py::arg("turn_index"),
             py::arg("scene_context"))
        .def("apply_planned_turn",
             [](Director& self, int turn_idx, const std::string& json_txt) -> DirectorOutput {
                 return self.apply_planned_turn(turn_idx, nlohmann::json::parse(json_txt));
             },
             py::arg("turn_index"), py::arg("json_txt"));

    // -- Validator --

    py::class_<Verdict>(m, "Verdict")
        .def(py::init<>())
        .def_readwrite("accepted", &Verdict::accepted)
        .def_readwrite("reason",   &Verdict::reason);

    py::class_<Validator>(m, "Validator")
        .def(py::init<const WorldGraph&>(), py::arg("graph"))
        .def("set_llm_callback",    &Validator::set_llm_callback)
        .def("set_search_callback", &Validator::set_search_callback)
        .def("set_dead_check",      &Validator::set_dead_check)
        .def("check", &Validator::check, py::arg("candidate"));

    // -- Annotator --

    py::class_<EntitySpan>(m, "EntitySpan")
        .def_readonly("start",    &EntitySpan::start)
        .def_readonly("end_",     &EntitySpan::end)
        .def_readonly("text",     &EntitySpan::text)
        .def_readonly("category", &EntitySpan::category);

    py::class_<Annotator>(m, "Annotator")
        .def(py::init<const Scene&>(), py::arg("scene"))
        .def("set_ner_callback", &Annotator::set_ner_callback)
        .def("annotate", &Annotator::annotate, py::arg("text"));

    // -- Scene Loop --

    // -- Weaver --

    py::class_<GraphAnalysis>(m, "GraphAnalysis")
        .def_readonly("live_node_count",  &GraphAnalysis::live_node_count)
        .def_readonly("active_edge_count", &GraphAnalysis::active_edge_count)
        .def_readonly("orphan_count",     &GraphAnalysis::orphan_count);

    py::class_<WeaveOp>(m, "WeaveOp")
        .def_readonly("from_id", &WeaveOp::from_id)
        .def_readonly("to_id",   &WeaveOp::to_id)
        .def_readonly("weight",  &WeaveOp::weight)
        .def_readonly("reason",  &WeaveOp::reason);

    py::class_<WeaveResult>(m, "WeaveResult")
        .def_readonly("connected",    &WeaveResult::connected)
        .def_readonly("disconnected", &WeaveResult::disconnected)
        .def_readonly("reweighted",   &WeaveResult::reweighted)
        .def_readonly("analysis",     &WeaveResult::analysis);

    py::class_<ExpiryOp>(m, "ExpiryOp")
        .def_readonly("id",     &ExpiryOp::id)
        .def_readonly("reason", &ExpiryOp::reason);

    py::class_<Weaver>(m, "Weaver")
        .def(py::init<WorldGraph&>(), py::arg("graph"))
        .def("set_llm_callback",       &Weaver::set_llm_callback)
        .def("set_local_llm_callback", &Weaver::set_local_llm_callback)
        .def("set_interval",           &Weaver::set_interval, py::arg("turns"))
        .def("should_weave",           &Weaver::should_weave, py::arg("turn_index"))
        .def("weave",                  &Weaver::weave,
             py::arg("turn_index"), py::arg("scene_context") = "")
        .def("weave_local",            &Weaver::weave_local,
             py::arg("turn_index"), py::arg("scene_context") = "")
        .def("rebuild_expiry_queue",   &Weaver::rebuild_expiry_queue,
             py::arg("priority_entities") = std::vector<std::string>{})
        .def("drain_expiry_queue",     &Weaver::drain_expiry_queue,
             py::arg("turn_index"),
             py::call_guard<py::gil_scoped_release>())
        .def("stop_expiry_drain",      &Weaver::stop_expiry_drain)
        .def("expiry_queue_empty",     &Weaver::expiry_queue_empty);

    m.def("analyze_graph", &analyze, py::arg("graph"));

    // -- TextDownsampler --

    py::class_<Snippet>(m, "Snippet")
        .def(py::init<>())
        .def_readwrite("text",         &Snippet::text)
        .def_readwrite("turn_start",   &Snippet::turn_start)
        .def_readwrite("turn_end",     &Snippet::turn_end)
        .def_readwrite("timestamp",    &Snippet::timestamp)
        .def_readwrite("promoted",     &Snippet::promoted)
        .def_readwrite("source_mip",   &Snippet::source_mip)
        .def_readwrite("merged_count", &Snippet::merged_count);

    py::class_<TextDownsampler>(m, "TextDownsampler")
        .def(py::init<>())
        .def("set_llm_callback",  &TextDownsampler::set_llm_callback)
        .def("has_llm_callback",  &TextDownsampler::has_llm_callback)
        .def("process_turn",      &TextDownsampler::process_turn,
             py::arg("messages"), py::arg("verbatim_tail") = 6)
        .def("render",            &TextDownsampler::render)
        .def("summarized_up_to",  &TextDownsampler::summarized_up_to)
        .def("to_json_str",       [](const TextDownsampler& td) {
            return td.to_json().dump();
        })
        .def_static("from_json_str", [](const std::string& s) {
            return TextDownsampler::from_json(nlohmann::json::parse(s));
        }, py::arg("json_str"));

    // -- Scene Loop --

    py::enum_<LoopState>(m, "LoopState")
        .value("Idle",             LoopState::Idle)
        .value("WaitingForInput",  LoopState::WaitingForInput)
        .value("ProcessingInput",  LoopState::ProcessingInput)
        .value("Weaving",          LoopState::Weaving)
        .value("BuildingPrompt",   LoopState::BuildingPrompt)
        .value("RunningLLM",       LoopState::RunningLLM)
        .value("AppendingResult",  LoopState::AppendingResult);

    py::class_<SceneLoop>(m, "SceneLoop")
        .def(py::init<>())
        .def("load_scene",                   &SceneLoop::load_scene)
        .def("submit_input",                 &SceneLoop::submit_input,
             py::call_guard<py::gil_scoped_release>())
        .def("state",                        &SceneLoop::state)
        .def("set_prompt_callback",          &SceneLoop::set_prompt_callback)
        .def("set_llm_callback",             &SceneLoop::set_llm_callback)
        .def("set_narrator_llm_callback",    &SceneLoop::set_narrator_llm_callback)
        .def("set_turn_complete_callback",   &SceneLoop::set_turn_complete_callback)
        .def("take_last_turn_outputs",       &SceneLoop::take_last_turn_outputs)
        .def("set_director",                 &SceneLoop::set_director, py::arg("director"))
        .def("last_director_output",         &SceneLoop::last_director_output)
        .def("set_weaver",                   &SceneLoop::set_weaver, py::arg("weaver"))
        .def("last_weave_result",            &SceneLoop::last_weave_result)
        .def("set_history_window",           &SceneLoop::set_history_window,
             py::arg("normal") = 3, py::arg("resume") = 10)
        .def("set_resuming",                 &SceneLoop::set_resuming, py::arg("v"))
        .def("set_saves_dir",               &SceneLoop::set_saves_dir, py::arg("dir"))
        .def("join_background",             &SceneLoop::join_background,
             py::call_guard<py::gil_scoped_release>())
        .def("take_completed_expiry_ops",   &SceneLoop::take_completed_expiry_ops);

    // -- Memory System --

    py::class_<MemorySystem>(m, "MemorySystem")
        .def(py::init<const std::string&>(), py::arg("scene_id"))
        .def("set_embed_callback",       &MemorySystem::set_embed_callback)
        .def("set_store_callback",       &MemorySystem::set_store_callback)
        .def("set_query_callback",       &MemorySystem::set_query_callback)
        .def("set_update_meta_callback", &MemorySystem::set_update_meta_callback)
        .def("set_get_by_meta_callback", &MemorySystem::set_get_by_meta_callback)
        .def("set_delete_callback",      &MemorySystem::set_delete_callback)
        .def("set_local_llm_callback",   &MemorySystem::set_local_llm_callback)
        .def("delete_nodes",             &MemorySystem::delete_nodes, py::arg("node_ids"))
        .def("store_node",              &MemorySystem::store_node,
             py::arg("node_id"), py::arg("fact"), py::arg("state"),
             py::arg("type"), py::arg("turn"))
        .def("search_nodes",            &MemorySystem::search_nodes,
             py::arg("query"), py::arg("top_k") = 10)
        .def("process_new_nodes",       &MemorySystem::process_new_nodes,
             py::arg("nodes"), py::arg("turn"))
        .def("sync_expired",            &MemorySystem::sync_expired,
             py::arg("expired_nodes"))
        .def("get_next_id",             &MemorySystem::get_next_id)
        .def("set_next_id",             &MemorySystem::set_next_id, py::arg("id"));
}
