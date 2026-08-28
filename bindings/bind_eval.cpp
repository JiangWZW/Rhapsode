#include "bindings.h"

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "rhapsode/eval/session_eval.h"
#include "rhapsode/eval/session_report.h"

namespace py = pybind11;
using namespace rhapsode;

void bind_eval(py::module_& m) {
    py::enum_<EndReason>(m, "EndReason")
        .value("MaxTurns", EndReason::MaxTurns)
        .value("TurnTimeout", EndReason::TurnTimeout)
        .value("ServerExit", EndReason::ServerExit)
        .value("WsError", EndReason::WsError)
        .value("TurnError", EndReason::TurnError);

    py::class_<SessionEvalConfig>(m, "SessionEvalConfig")
        .def(py::init<>())
        .def_readwrite("ws_host", &SessionEvalConfig::ws_host)
        .def_readwrite("ws_port", &SessionEvalConfig::ws_port)
        .def_readwrite("ws_path", &SessionEvalConfig::ws_path)
        .def_readwrite("server_cmd", &SessionEvalConfig::server_cmd)
        .def_readwrite("saves_dir", &SessionEvalConfig::saves_dir)
        .def_readwrite("out_dir", &SessionEvalConfig::out_dir)
        .def_readwrite("max_turns", &SessionEvalConfig::max_turns)
        .def_readwrite("turn_timeout_s", &SessionEvalConfig::turn_timeout_s)
        .def_readwrite("open_timeout_s", &SessionEvalConfig::open_timeout_s);

    py::class_<ReliabilityMetrics>(m, "ReliabilityMetrics")
        .def_readonly("turns_completed", &ReliabilityMetrics::turns_completed)
        .def_readonly("turns_requested", &ReliabilityMetrics::turns_requested)
        .def_readonly("ready_ms", &ReliabilityMetrics::ready_ms)
        .def_readonly("idle_ms", &ReliabilityMetrics::idle_ms)
        .def_readonly("turn_ms", &ReliabilityMetrics::turn_ms)
        .def_readonly("timeouts", &ReliabilityMetrics::timeouts)
        .def_readonly("errors", &ReliabilityMetrics::errors)
        .def_readonly("server_exit_code", &ReliabilityMetrics::server_exit_code)
        .def_readonly("log_markers", &ReliabilityMetrics::log_markers);

    py::class_<NarrativeMetrics>(m, "NarrativeMetrics")
        .def_readonly("empty_beats", &NarrativeMetrics::empty_beats)
        .def_readonly("repetition_score", &NarrativeMetrics::repetition_score)
        .def_readonly("length_collapse", &NarrativeMetrics::length_collapse)
        .def_readonly("cast_gaps", &NarrativeMetrics::cast_gaps)
        .def_readonly("findings", &NarrativeMetrics::findings);

    py::class_<SessionReport>(m, "SessionReport")
        .def_readonly("end_reason", &SessionReport::end_reason)
        .def_readonly("reliability", &SessionReport::reliability)
        .def_readonly("narrative", &SessionReport::narrative)
        .def_readonly("critique", &SessionReport::critique)
        .def_static(
            "from_run_dir",
            [](const std::string& dir, py::object critique_llm) {
                LLMCallback cb;
                if (!critique_llm.is_none()) {
                    cb = [critique_llm](const std::string& prompt) {
                        py::gil_scoped_acquire gil;
                        return critique_llm(prompt).cast<std::string>();
                    };
                }
                py::gil_scoped_release release;
                return SessionReport::from_run_dir(dir, std::move(cb));
            },
            py::arg("dir"), py::arg("critique_llm") = py::none())
        .def("write", &SessionReport::write, py::arg("dir"));

    py::class_<SessionEvalRunner>(m, "SessionEvalRunner")
        .def(py::init<SessionEvalConfig>(), py::arg("config"))
        .def(
            "set_player_llm",
            [](SessionEvalRunner& self, py::function fn) {
                self.set_player_llm([fn](const std::string& prompt) {
                    py::gil_scoped_acquire gil;
                    return fn(prompt).cast<std::string>();
                });
            },
            py::arg("callback"))
        .def(
            "set_critique_llm",
            [](SessionEvalRunner& self, py::function fn) {
                self.set_critique_llm([fn](const std::string& prompt) {
                    py::gil_scoped_acquire gil;
                    return fn(prompt).cast<std::string>();
                });
            },
            py::arg("callback"))
        .def(
            "run",
            [](SessionEvalRunner& self) {
                py::gil_scoped_release release;
                return self.run();
            });
}
