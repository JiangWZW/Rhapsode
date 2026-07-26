#include <pybind11/pybind11.h>

#include "bindings.h"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.doc() = "Rhapsode C++ core bindings";

    // Keep registration order stable: some properties refer to types whose
    // bindings are installed by the following domain function.
    bind_story(m);
    bind_graph(m);
    bind_runtime(m);
    bind_memory(m);
    bind_eval(m);
}
