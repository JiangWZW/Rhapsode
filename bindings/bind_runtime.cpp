#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

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
        .def("process_turn",      &TextDownsampler::process_turn,
             py::arg("messages"), py::arg("llm_callback"),
             py::arg("verbatim_tail") = 6)
        .def("render",            &TextDownsampler::render)
        .def("summarized_up_to",  &TextDownsampler::summarized_up_to)
        .def("to_json_str",       [](const TextDownsampler& td) {
            return td.to_json().dump();
        })
        .def_static("from_json_str", [](const std::string& s) {
            return TextDownsampler::from_json(nlohmann::json::parse(s));
        }, py::arg("json_str"));

}
