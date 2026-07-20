#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/scene.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/text_downsampler.h"
#include "rhapsode/weaver.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_runtime(py::module_& m) {
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
        .def(py::init<WorldGraph&>(), py::arg("graph"), py::keep_alive<1, 2>())
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
        .def("load_scene",                   &SceneLoop::load_scene,
             py::keep_alive<1, 2>())
        .def("submit_input",                 &SceneLoop::submit_input,
             py::call_guard<py::gil_scoped_release>())
        .def("submit_autonomous",            &SceneLoop::submit_autonomous,
             py::call_guard<py::gil_scoped_release>())
        .def("state",                        &SceneLoop::state)
        .def("set_llm_callback",             &SceneLoop::set_llm_callback)
        .def("set_narrator_llm_callback",    &SceneLoop::set_narrator_llm_callback)
        .def("set_turn_complete_callback",   &SceneLoop::set_turn_complete_callback)
        .def("take_last_turn_outputs",       &SceneLoop::take_last_turn_outputs)
        .def("set_director",                 &SceneLoop::set_director, py::arg("director"),
             py::keep_alive<1, 2>())
        .def("last_director_output",         &SceneLoop::last_director_output)
        .def("set_weaver",                   &SceneLoop::set_weaver, py::arg("weaver"),
             py::keep_alive<1, 2>())
        .def("last_weave_result",            &SceneLoop::last_weave_result)
        .def("set_history_window",           &SceneLoop::set_history_window,
             py::arg("normal") = 3, py::arg("resume") = 10)
        .def("set_resuming",                 &SceneLoop::set_resuming, py::arg("v"))
        .def("set_saves_dir",               &SceneLoop::set_saves_dir, py::arg("dir"))
        .def("join_background",             &SceneLoop::join_background,
             py::call_guard<py::gil_scoped_release>())
        .def("take_completed_expiry_ops",   &SceneLoop::take_completed_expiry_ops);
}
