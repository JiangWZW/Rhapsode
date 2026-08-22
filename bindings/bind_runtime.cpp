#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/weaver.h"

namespace py = pybind11;
using namespace rhapsode;

namespace {

// Compatibility owner for the historical Python Weaver(graph) API. The core
// Weaver service itself retains no WorldGraph reference.
struct BoundWeaver {
    explicit BoundWeaver(WorldGraph& value) : graph(&value) {}

    WorldGraph* graph;
    Weaver service;
};

}  // namespace

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

    py::class_<BoundWeaver>(m, "Weaver")
        .def(py::init<WorldGraph&>(), py::arg("graph"), py::keep_alive<1, 2>())
        .def("set_llm_callback", [](BoundWeaver& self, LLMCallback callback) {
            self.service.set_llm_callback(std::move(callback));
        })
        .def("set_interval", [](BoundWeaver& self, int turns) {
            self.service.set_interval(turns);
        }, py::arg("turns"))
        .def("should_weave", [](const BoundWeaver& self, int turn_index) {
            return self.service.should_weave(turn_index);
        }, py::arg("turn_index"))
        .def("weave", [](BoundWeaver& self, int turn_index,
                          const std::string& scene_context) {
            return self.service.weave(*self.graph, turn_index, scene_context);
        }, py::arg("turn_index"), py::arg("scene_context") = "")
        .def("rebuild_expiry_queue", [](BoundWeaver& self,
                                         const std::vector<std::string>& priority) {
            self.service.rebuild_expiry_queue(*self.graph, priority);
        }, py::arg("priority_entities") = std::vector<std::string>{})
        .def("drain_expiry_queue", [](BoundWeaver& self, int turn_index) {
            return self.service.drain_expiry_queue(*self.graph, turn_index);
        }, py::arg("turn_index"),
             py::call_guard<py::gil_scoped_release>())
        .def("stop_expiry_drain", [](BoundWeaver& self) {
            self.service.stop_expiry_drain();
        })
        .def("expiry_queue_empty", [](const BoundWeaver& self) {
            return self.service.expiry_queue_empty();
        });

    m.def("analyze_graph", &analyze, py::arg("graph"));

}
