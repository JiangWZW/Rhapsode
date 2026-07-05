#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
#include "rhapsode/node.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

struct Verdict {
    bool accepted = true;
    std::string reason;
};

using SearchCallback = std::function<std::vector<std::uint64_t>(const std::string&, int)>;
using DeadCheckCallback = std::function<std::vector<std::string>()>;

class Validator {
public:
    explicit Validator(const WorldGraph& graph);

    void set_llm_callback(LLMCallback cb);
    void set_search_callback(SearchCallback cb);
    void set_dead_check(DeadCheckCallback cb);

    Verdict check(const Node& candidate) const;

private:
    const WorldGraph& graph_;
    LLMCallback llm_cb_;
    SearchCallback search_cb_;
    DeadCheckCallback dead_check_cb_;

    std::vector<const Node*> gather_context(const Node& candidate) const;

    std::string build_data_section(
        const Node& candidate,
        const std::vector<const Node*>& context,
        const std::vector<std::string>& dead_entities) const;

    std::string try_llm_call(const std::string& prompt) const;
};

}  // namespace rhapsode
