#include "rhapsode/world_graph.h"

#include "rhapsode/json_util.h"

#include <sstream>

namespace rhapsode {

namespace {

// Render a fact as a graphviz label: full text (no truncation), soft-wrapped at
// word boundaries so node boxes stay readable, and always valid UTF-8.
std::string dot_escape(const std::string& s, std::size_t wrap_col = 45) {
    const std::string clean = sanitize_utf8(s);  // never split a multibyte char
    std::string out;
    out.reserve(clean.size() + 16);
    std::size_t col = 0;
    for (char c : clean) {
        switch (c) {
            case '"':  out += "\\\""; ++col; break;
            case '\\': out += "\\\\"; ++col; break;
            case '\r': break;
            case '\n': out += "\\n"; col = 0; break;
            case ' ':
                // wrap at the first space past the column limit
                if (col >= wrap_col) { out += "\\n"; col = 0; }
                else                 { out += ' ';   ++col; }
                break;
            default:   out += c; ++col; break;
        }
    }
    return out;
}

}  // namespace

std::string WorldGraph::to_dot() const {
    std::ostringstream os;
    os << "digraph WorldGraph {\n"
       << "  rankdir=LR;\n"
       << "  bgcolor=\"#1e1e2e\";\n"
       << "  node [shape=box, style=\"filled,rounded\", fontname=\"Segoe UI\", fontsize=10];\n"
       << "  edge [fontname=\"Segoe UI\", fontsize=8, fontcolor=\"#cdd6f4\"];\n\n";

    for (const auto& [id, v] : id_to_vertex_) {
        const Node& n = graph_[v];
        const char* fill  = "#585b70";
        const char* font  = "#cdd6f4";
        const char* border = "#6c7086";
        switch (n.state) {
            case NodeState::Dormant:
                break;
            case NodeState::Foreshadowed:
                fill = "#f9e2af"; font = "#1e1e2e"; border = "#f2c678"; break;
            case NodeState::Active:
                if (n.valid_until != -1) {
                    fill = "#89b4fa"; font = "#1e1e2e"; border = "#6a9bf5";
                } else {
                    fill = "#a6e3a1"; font = "#1e1e2e"; border = "#74c76e";
                }
                break;
        }
        os << "  n" << n.id
           << " [label=\"[" << n.id << "] " << dot_escape(n.fact) << "\""
           << ", fillcolor=\"" << fill << "\""
           << ", fontcolor=\"" << font << "\""
           << ", color=\"" << border << "\""
           << "];\n";
    }

    os << "\n";

    auto [edge_it, edge_end] = boost::edges(graph_);
    for (; edge_it != edge_end; ++edge_it) {
        Vertex src = boost::source(*edge_it, graph_);
        Vertex dst = boost::target(*edge_it, graph_);
        const EdgeData& d = graph_[*edge_it];

        const char* style = d.active ? "solid" : "dashed";

        os << "  n" << graph_[src].id << " -> n" << graph_[dst].id
           << " [color=\"#a6adc8\""
           << ", style=" << style
           << "];\n";
    }

    os << "\n  label=\"Rhapsode World Graph\";\n"
       << "  fontname=\"Segoe UI\"; fontsize=14; fontcolor=\"#cdd6f4\";\n"
       << "}\n";
    return os.str();
}

}  // namespace rhapsode
