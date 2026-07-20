#pragma once

#include <pybind11/pybind11.h>

void bind_story(pybind11::module_& m);
void bind_graph(pybind11::module_& m);
void bind_runtime(pybind11::module_& m);
void bind_memory(pybind11::module_& m);
