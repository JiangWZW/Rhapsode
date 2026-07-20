#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/memory_system.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_memory(py::module_& m) {
    py::class_<MemorySystem>(m, "MemorySystem")
        .def(py::init<const std::string&>(), py::arg("scene_id"))
        .def("set_embed_callback",       &MemorySystem::set_embed_callback)
        .def("set_store_callback",       &MemorySystem::set_store_callback)
        .def("set_query_callback",       &MemorySystem::set_query_callback)
        .def("set_update_meta_callback", &MemorySystem::set_update_meta_callback)
        .def("set_delete_callback",      &MemorySystem::set_delete_callback)
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
