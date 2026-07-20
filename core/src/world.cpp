#include "rhapsode/world.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace rhapsode {

// -- Roster ------------------------------------------------------------------

Character* World::find_character(const std::string& name) {
    for (auto& c : characters)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* World::find_character(const std::string& name) const {
    for (const auto& c : characters)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

// -- Death scan --------------------------------------------------------------

std::vector<DeathCandidate> World::scan_death_candidates() const {
    std::vector<DeathCandidate> candidates;
    auto all_nodes = world_graph.all_nodes(true);

    static const std::vector<std::string> death_keywords = {
        "dead", "dies", "died", "kills", "killed", "killing",
        "corpse", "slain", "perished", "executed", "murdered",
        "succumbed", "fatal"
    };

    auto has_death_keyword = [&](const std::string& text_lower) {
        for (const auto& kw : death_keywords)
            if (text_lower.find(kw) != std::string::npos) return true;
        return false;
    };

    auto mentions_character = [&](const Node& n, const std::string& name,
                                  const std::string& name_lower) {
        for (const auto& ent : n.entities)
            if (str::iequals(ent, name)) return true;
        auto fl = str::to_lower(n.fact);
        if (fl.find(name_lower) != std::string::npos) return true;
        auto cl = str::to_lower(n.active_ctx);
        return cl.find(name_lower) != std::string::npos;
    };

    for (const auto& ch : characters) {
        if (ch.is_player || ch.dead) continue;

        std::string name_lower = str::to_lower(ch.name);
        bool has_death_hit = false;

        for (const auto& n : all_nodes) {
            if (n.state != NodeState::Active) continue;
            if (!mentions_character(n, ch.name, name_lower)) continue;

            std::string fact_lower = str::to_lower(n.fact);
            std::string ctx_lower  = str::to_lower(n.active_ctx);
            if (has_death_keyword(fact_lower) || has_death_keyword(ctx_lower))
                has_death_hit = true;
        }

        if (!has_death_hit) continue;

        DeathCandidate dc;
        dc.character_name = ch.name;
        for (const auto& n : all_nodes) {
            if (n.state != NodeState::Active) continue;
            if (!mentions_character(n, ch.name, name_lower)) continue;
            dc.evidence.push_back(n.fact);
        }
        candidates.push_back(std::move(dc));
    }

    return candidates;
}

void World::route_perceptions(const std::string& scene_id,
                              const std::vector<Node>& nodes,
                              int turn) {
    int deliveries = 0;
    std::unordered_set<std::string> minds;
    auto route_to = [&](const std::string& name, const Node& node) {
        auto it = character_memories.find(name);
        if (it != character_memories.end()) {
            it->second.route_fact(node.fact, node.entities, turn);
            ++deliveries;
            minds.insert(it->first);
        }
    };

    for (const auto& node : nodes) {
        if (!node.audience.empty()) {
            for (const auto& name : node.audience) {
                route_to(name, node);
            }
        } else {
            for (const auto& character : characters) {
                if (character.is_player || character.dead || !character.in_scene(scene_id)) {
                    continue;
                }
                route_to(character.name, node);
            }
        }
    }

    log() << "  [perceive] " << nodes.size() << " new_node(s) -> " << deliveries
          << " perception(s) routed to " << minds.size() << " mind(s)\n" << std::flush;
}

void World::reflect_perceptions(int turn) {
    for (auto& [name, memory] : character_memories) {
        std::string description;
        for (const auto& character : characters) {
            if (character.name == name) {
                description = character.description;
                break;
            }
        }
        memory.reflect_perceptions(turn, description);
    }
}

bool World::mark_character_dead(const std::string& name) {
    for (auto& character : characters) {
        if (character.name == name) {
            character.dead = true;
            character.scene_ids.clear();
            return true;
        }
    }
    return false;
}

// -- Staged lifecycle decisions ----------------------------------------------

std::string World::stage_fork(const std::string& source_scene_id,
                              const std::string& driving_intention,
                              const std::vector<std::string>& cast) {
    LifecycleOp op;
    op.kind = LifecycleKind::Fork;
    op.source_scene_id = source_scene_id;
    op.driving_intention = driving_intention;
    op.cast = cast;
    pending_ops_.push_back(std::move(op));

    nlohmann::json ack;
    ack["ok"] = true;
    ack["staged"] = "fork_scene";
    ack["driving_intention"] = driving_intention;
    ack["cast"] = cast;
    ack["note"] = "The new storyline will begin after this beat is accepted.";
    return ack.dump();
}

std::string World::stage_conclude(const std::string& source_scene_id,
                                  const std::string& reason) {
    LifecycleOp op;
    op.kind = LifecycleKind::Conclude;
    op.source_scene_id = source_scene_id;
    op.reason = reason;
    pending_ops_.push_back(std::move(op));

    nlohmann::json ack;
    ack["ok"] = true;
    ack["staged"] = "conclude_scene";
    ack["scene_id"] = source_scene_id;
    ack["reason"] = reason;
    return ack.dump();
}

std::string World::stage_merge(const std::string& source_scene_id,
                               const std::string& into_scene_id) {
    LifecycleOp op;
    op.kind = LifecycleKind::Merge;
    op.source_scene_id = source_scene_id;
    op.target_scene_id = into_scene_id;
    pending_ops_.push_back(std::move(op));

    nlohmann::json ack;
    ack["ok"] = true;
    ack["staged"] = "merge_scene";
    ack["from"] = source_scene_id;
    ack["into"] = into_scene_id;
    return ack.dump();
}

std::string World::stage_exit(const std::string& source_scene_id,
                              const std::vector<std::string>& cast) {
    LifecycleOp op;
    op.kind = LifecycleKind::Exit;
    op.source_scene_id = source_scene_id;
    op.cast = cast;
    pending_ops_.push_back(std::move(op));

    nlohmann::json ack;
    ack["ok"] = true;
    ack["staged"] = "exit";
    ack["cast"] = cast;
    return ack.dump();
}

std::vector<LifecycleOp> World::take_pending_ops() {
    return std::exchange(pending_ops_, {});
}

// -- Narrator tool-use queries -----------------------------------------------

namespace {

nlohmann::json node_to_json(const Node& n) {
    nlohmann::json j;
    j["id"]          = n.id;
    j["fact"]        = n.fact;
    j["state"]       = to_string(n.state);
    j["type"]        = n.type;
    j["valid_until"] = n.valid_until;
    j["weight"]      = n.weight;
    j["created_at"]  = n.created_at;
    j["entities"]    = n.entities;
    j["chain_to"]    = n.related_to;
    return j;
}

std::unordered_set<std::string> find_matching_entities(const std::vector<Node>& all,
                                                       const std::string& query)
{
    std::unordered_set<std::string> matched;
    for (const auto& node : all) {
        for (const auto& ent : node.entities) {
            if (str::iequals(ent, query))
                matched.insert(str::to_lower(ent));
        }
    }
    return matched;
}

std::vector<const Node*> collect_entity_timeline(const std::vector<Node>& all,
                                                 const std::string& entity_lower,
                                                 std::size_t max_count = 20)
{
    std::vector<const Node*> timeline;
    for (const auto& node : all) {
        for (const auto& e : node.entities) {
            if (str::to_lower(e) == entity_lower) {
                timeline.push_back(&node);
                break;
            }
        }
    }
    std::sort(timeline.begin(), timeline.end(),
        [](const Node* a, const Node* b) { return a->created_at < b->created_at; });
    if (timeline.size() > max_count)
        timeline.resize(max_count);
    return timeline;
}

std::vector<const Node*> find_fact_substring_matches(const std::vector<Node>& all,
                                                     const std::string& query_lower,
                                                     std::size_t max_count = 15)
{
    std::vector<const Node*> matches;
    for (const auto& node : all) {
        if (str::to_lower(node.fact).find(query_lower) != std::string::npos)
            matches.push_back(&node);
    }
    std::sort(matches.begin(), matches.end(),
        [](const Node* a, const Node* b) { return a->created_at > b->created_at; });
    if (matches.size() > max_count)
        matches.resize(max_count);
    return matches;
}

}  // namespace

std::string World::tool_query_graph(const std::string& query) const {
    const std::string query_lower = str::to_lower(query);
    auto all = world_graph.all_nodes(true);

    auto matched_entities = find_matching_entities(all, query);
    if (!matched_entities.empty()) {
        nlohmann::json chains = nlohmann::json::array();
        for (const auto& ent_lower : matched_entities) {
            auto timeline = collect_entity_timeline(all, ent_lower);

            nlohmann::json chain;
            std::string entity_name = query;
            for (const auto* n : timeline) {
                for (const auto& e : n->entities) {
                    if (str::to_lower(e) == ent_lower) {
                        entity_name = e;
                        break;
                    }
                }
            }
            chain["entity"] = entity_name;
            nlohmann::json tl = nlohmann::json::array();
            for (const auto* n : timeline)
                tl.push_back(node_to_json(*n));
            chain["timeline"] = std::move(tl);
            chains.push_back(std::move(chain));
        }

        nlohmann::json result;
        result["chains"] = std::move(chains);
        result["matches"] = nlohmann::json::array();
        return result.dump();
    }

    auto matches = find_fact_substring_matches(all, query_lower);
    nlohmann::json ms = nlohmann::json::array();
    for (const auto* n : matches)
        ms.push_back(node_to_json(*n));

    nlohmann::json result;
    result["chains"] = nlohmann::json::array();
    result["matches"] = std::move(ms);
    return result.dump();
}

std::string World::tool_query_mind(const std::string& character) const {
    const Character* ch = find_character(character);

    auto it = character_memories.end();
    if (ch) {
        it = character_memories.find(ch->name);
    }
    if (it == character_memories.end()) {
        for (auto mem_it = character_memories.begin();
             mem_it != character_memories.end(); ++mem_it) {
            if (str::iequals(mem_it->first, character)) {
                it = mem_it;
                break;
            }
        }
    }

    if (it == character_memories.end()) {
        nlohmann::json err;
        err["error"] = "character not found";
        return err.dump();
    }

    nlohmann::json result;
    result["character"] = it->first;
    if (ch) {
        result["voice"] = ch->build_prompt__dialogue_voice();
    }
    result["thoughts"] = it->second.render_thoughts({});
    return result.dump();
}

// -- Scenario bootstrap ------------------------------------------------------

void World::seed_from_scenario(const nlohmann::json& j) {
    nlohmann::json graph_j;
    graph_j["next_id"] = 1;
    graph_j["nodes"] = j.value("nodes", nlohmann::json::array());
    graph_j["edges"] = j.value("edges", nlohmann::json::array());
    world_graph = WorldGraph::from_json(graph_j);

    // Bootstrap CharacterMemory from initial_memory in character definitions.
    for (const auto& ch_j : j.value("characters", nlohmann::json::array())) {
        std::string name = ch_j.value("name", "");
        if (name.empty() || ch_j.value("is_player", false)) continue;
        if (!ch_j.contains("initial_memory")) continue;

        CharacterMemory mem(name);
        const auto& im = ch_j["initial_memory"];

        const auto& beliefs_j = im.value("beliefs", nlohmann::json::array());
        std::vector<std::uint64_t> seeded_ids(beliefs_j.size(), 0);
        for (std::size_t bi = 0; bi < beliefs_j.size(); ++bi) {
            const auto& bj = beliefs_j[bi];
            std::vector<std::string> about;
            if (bj.contains("about")) {
                const auto& aj = bj["about"];
                if (aj.is_string())     about.push_back(aj.get<std::string>());
                else if (aj.is_array()) for (const auto& a : aj) about.push_back(a.get<std::string>());
            }
            float w = CharacterMemory::kAuthoredSeedWeight;
            if (bj.contains("weight")) w = bj["weight"].get<float>();
            // A belief may be authored as a forward "intention" (the drive a
            // storyline is about), distinguished from a memory/view.
            std::string btype = bj.value("intention", false)
                                    ? std::string("intention")
                                    : bj.value("type", std::string("belief"));
            seeded_ids[bi] = mem.seed_belief(bj.value("content", ""), about, 0, w, btype);
        }

        for (std::size_t bi = 0; bi < beliefs_j.size(); ++bi) {
            if (!beliefs_j[bi].contains("tension_with")) continue;
            const auto& tw = beliefs_j[bi]["tension_with"];
            auto link = [&](long long other) {
                if (other >= 0 && static_cast<std::size_t>(other) < seeded_ids.size())
                    mem.link_tension(seeded_ids[bi],
                                     seeded_ids[static_cast<std::size_t>(other)], 0);
            };
            if (tw.is_number())     link(tw.get<long long>());
            else if (tw.is_array()) for (const auto& o : tw) if (o.is_number()) link(o.get<long long>());
        }

        for (const auto& ctx : im.value("context", nlohmann::json::array()))
            mem.seed_belief(ctx.get<std::string>(), {name}, 0);

        character_memories.emplace(name, std::move(mem));
    }
}

// -- Persistence (durable substrate) -----------------------------------------

std::string World::save_path(const std::string& saves_dir) {
    return saves_dir + "/world.json";
}

bool World::has_save(const std::string& saves_dir) const {
    return std::filesystem::exists(save_path(saves_dir));
}

nlohmann::json World::to_json() const {
    nlohmann::json j;
    j["memory_next_id"] = memory_ ? memory_->get_next_id() : 0;
    j["world_graph"]    = world_graph.to_json();
    j["characters"]     = characters;

    nlohmann::json cm_j;
    for (const auto& [name, mem] : character_memories)
        cm_j[name] = mem.to_json();
    j["character_memories"] = std::move(cm_j);
    return j;
}

World World::from_json(const nlohmann::json& j) {
    World w;
    if (j.contains("world_graph")) {
        w.world_graph = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        w.world_graph = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    }
    if (j.contains("characters") && j["characters"].is_array())
        w.characters = j["characters"].get<std::vector<Character>>();
    if (j.contains("character_memories") && j["character_memories"].is_object()) {
        for (auto& [name, cm_j] : j["character_memories"].items())
            w.character_memories.insert_or_assign(name, CharacterMemory::from_json(cm_j));
    }
    return w;
}

void World::load_save(const std::string& saves_dir) {
    std::ifstream in(save_path(saves_dir));
    if (!in.is_open())
        throw std::runtime_error("No world save in: " + saves_dir);

    nlohmann::json j;
    in >> j;

    if (j.contains("world_graph")) {
        world_graph = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        world_graph = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    } else {
        throw std::runtime_error("World save is missing world_graph/node_pool data");
    }

    if (j.contains("characters") && j["characters"].is_array())
        characters = j["characters"].get<std::vector<Character>>();

    if (memory_)
        memory_->set_next_id(j.value("memory_next_id", 0));

    if (j.contains("character_memories") && j["character_memories"].is_object()) {
        character_memories.clear();
        for (auto& [name, cm_j] : j["character_memories"].items())
            character_memories.insert_or_assign(name, CharacterMemory::from_json(cm_j));
    }
}

void World::save(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);
    std::ofstream out(save_path(saves_dir));
    if (!out.is_open())
        throw std::runtime_error("Cannot write world save in: " + saves_dir);
    out << to_json().dump(2);
}

void World::delete_save(const std::string& saves_dir) const {
    std::filesystem::remove(save_path(saves_dir));
}

} // namespace rhapsode
