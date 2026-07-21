#include "rhapsode/world.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace rhapsode {

// -- Roster ------------------------------------------------------------------

Character* World::find_character_mutable(const std::string& name) {
    for (auto& c : characters_)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* World::find_character(const std::string& name) const {
    for (const auto& c : characters_)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* World::find_in_scene(const std::string& scene_id,
                                      const std::string& name) const {
    const Character* character = find_character(name);
    return character && !character->dead && character->in_scene(scene_id)
        ? character : nullptr;
}

Character& World::enter_character(const std::string& scene_id, Character character) {
    if (Character* existing = find_character_mutable(character.name)) {
        if (existing->dead) {
            log() << "  [cast] " << existing->name
                  << " is dead -- ignoring re-entry\n";
            return *existing;
        }
        const bool was_off = !existing->in_scene(scene_id);
        existing->join_scene(scene_id);
        if (existing->description.empty() && !character.description.empty())
            existing->description = std::move(character.description);
        if (existing->dialogue_instructions.empty() &&
            !character.dialogue_instructions.empty())
            existing->dialogue_instructions = std::move(character.dialogue_instructions);
        if (was_off)
            log() << "  [cast] " << existing->name
                  << " re-enters (was off-stage)\n";
        return *existing;
    }

    if (!scene_id.empty()) character.join_scene(scene_id);
    characters_.push_back(std::move(character));
    Character& added = characters_.back();
    log() << "  [cast] NEW " << added.name
          << " | role=" << (added.role.empty() ? "?" : added.role)
          << " | \"" << added.description.substr(0, 80) << "\"\n";

    if (!added.is_player && character_memories_.find(added.name) == character_memories_.end()) {
        character_memories_.emplace(added.name, CharacterMemory(added.name));
        log() << "  [cast]   memory created (empty -- learns by perception)\n";
    }
    return added;
}

bool World::leave_character(const std::string& scene_id, const std::string& name) {
    Character* character = find_character_mutable(name);
    if (!character || !character->in_scene(scene_id)) return false;
    character->leave_scene(scene_id);
    return true;
}

void World::ensure_characters_present(
    const std::string& scene_id,
    const std::vector<std::string>& canonical_names) {
    std::unordered_set<std::string> resolved_names;
    resolved_names.reserve(canonical_names.size());
    for (const auto& name : canonical_names)
        resolved_names.insert(str::to_lower(name));

    for (auto& character : characters_) {
        if (character.is_player || character.dead) continue;
        if (resolved_names.count(str::to_lower(character.name)) > 0 &&
            !character.in_scene(scene_id)) {
            character.join_scene(scene_id);
            log() << "  [cast] " << character.name << " enters (in active_cast)\n";
        }
    }
}

void World::move_scene_members(const std::string& from_scene_id,
                               const std::string& into_scene_id,
                               const std::vector<std::string>& names) {
    std::unordered_set<std::string> selected;
    for (const auto& name : names) selected.insert(str::to_lower(name));
    for (auto& character : characters_) {
        if (!character.in_scene(from_scene_id)) continue;
        if (!selected.empty() && selected.count(str::to_lower(character.name)) == 0) continue;
        if (!names.empty() && character.is_player) continue;
        character.join_scene(into_scene_id);
        character.leave_scene(from_scene_id);
    }
}

void World::clear_scene_membership(const std::string& scene_id) {
    for (auto& character : characters_) character.leave_scene(scene_id);
}

void World::set_character_memory(CharacterMemory memory) {
    character_memories_.insert_or_assign(memory.name(), std::move(memory));
}

bool World::seed_character_intention(const std::string& character,
                                     const std::string& intention,
                                     const std::vector<std::string>& subjects,
                                     int created_at) {
    const Character* canonical = find_character(character);
    if (!canonical) return false;
    auto it = character_memories_.find(canonical->name);
    if (it == character_memories_.end()) return false;
    it->second.seed_belief(intention, subjects, created_at,
                           CharacterMemory::kAuthoredSeedWeight, "intention");
    return true;
}

std::vector<std::uint64_t> World::revert_to_turn(int target_turn) {
    const auto removed_ids = world_graph_.revert_to_turn(target_turn);

    characters_.erase(
        std::remove_if(characters_.begin(), characters_.end(),
            [target_turn](const Character& c) { return c.created_at >= target_turn; }),
        characters_.end());
    for (auto& character : characters_)
        if (!character.is_player) character.dead = false;

    for (auto it = character_memories_.begin(); it != character_memories_.end();) {
        const bool exists = std::any_of(characters_.begin(), characters_.end(),
            [&](const Character& c) { return c.name == it->first; });
        it = exists ? std::next(it) : character_memories_.erase(it);
    }
    return removed_ids;
}

// -- Death scan --------------------------------------------------------------

std::vector<DeathCandidate> World::scan_death_candidates() const {
    std::vector<DeathCandidate> candidates;
    auto all_nodes = world_graph_.all_nodes(true);

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

    for (const auto& ch : characters_) {
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
        auto it = character_memories_.find(name);
        if (it != character_memories_.end()) {
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
            for (const auto& character : characters_) {
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

void World::reflect_perceptions(int turn, const LLMCallback& llm_callback) {
    for (auto& [name, memory] : character_memories_) {
        std::string description;
        for (const auto& character : characters_) {
            if (character.name == name) {
                description = character.description;
                break;
            }
        }
        memory.reflect_perceptions(turn, description, llm_callback);
    }
}

bool World::mark_character_dead(const std::string& name) {
    for (auto& character : characters_) {
        if (character.name == name) {
            character.dead = true;
            character.scene_ids.clear();
            return true;
        }
    }
    return false;
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
    auto all = world_graph_.all_nodes(true);

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

    auto it = character_memories_.end();
    if (ch) {
        it = character_memories_.find(ch->name);
    }
    if (it == character_memories_.end()) {
        for (auto mem_it = character_memories_.begin();
             mem_it != character_memories_.end(); ++mem_it) {
            if (str::iequals(mem_it->first, character)) {
                it = mem_it;
                break;
            }
        }
    }

    if (it == character_memories_.end()) {
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

void World::seed_from_scenario(const nlohmann::json& j,
                               const std::string& root_scene_id) {
    characters_.clear();
    character_memories_.clear();
    for (const auto& character_json : j.value("characters", nlohmann::json::array())) {
        Character character = character_json.get<Character>();
        const bool authored_on = character_json.value(
            "on_stage", character_json.value("is_player", false));
        if (authored_on && !root_scene_id.empty()) character.join_scene(root_scene_id);
        characters_.push_back(std::move(character));
    }

    nlohmann::json graph_j;
    graph_j["next_id"] = 1;
    graph_j["nodes"] = j.value("nodes", nlohmann::json::array());
    graph_j["edges"] = j.value("edges", nlohmann::json::array());
    world_graph_ = WorldGraph::from_json(graph_j);

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

        set_character_memory(std::move(mem));
    }
}

nlohmann::json World::to_json() const {
    nlohmann::json j;
    j["world_graph"]    = world_graph_.to_json();
    j["characters"]     = characters_;

    nlohmann::json cm_j;
    for (const auto& [name, mem] : character_memories_)
        cm_j[name] = mem.to_json();
    j["character_memories"] = std::move(cm_j);
    return j;
}

World World::from_json(const nlohmann::json& j) {
    World w;
    if (j.contains("world_graph")) {
        w.world_graph_ = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        w.world_graph_ = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    }
    if (j.contains("characters") && j["characters"].is_array())
        w.characters_ = j["characters"].get<std::vector<Character>>();
    if (j.contains("character_memories") && j["character_memories"].is_object()) {
        for (auto& [name, cm_j] : j["character_memories"].items())
            w.character_memories_.insert_or_assign(name, CharacterMemory::from_json(cm_j));
    }
    return w;
}

} // namespace rhapsode
