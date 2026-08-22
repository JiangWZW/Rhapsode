#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/weaver.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_runtime(py::module_& m) {
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

    m.def("analyze_graph", &analyze, py::arg("graph"));
}
