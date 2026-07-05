#include "rhapsode/scene.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/str_util.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <unordered_set>

namespace rhapsode {

namespace {

void migrate_character_lines_to_dialogue(History& history, History& dialogue) {
    std::vector<SceneMessage> kept;
    kept.reserve(history.size());
    for (const auto& msg : history.messages()) {
        if (msg.metadata.value("scene_kind", std::string{}) == "character") {
            SceneMessage copy = msg;
            dialogue.append(std::move(copy));
        } else {
            kept.push_back(msg);
        }
    }
    history.clear();
    for (auto& m : kept)
        history.append(std::move(m));
}

}  // namespace

// -- Character lifecycle (theatre model) --

Character& Scene::enter_character(Character ch) {
    for (auto& existing : characters) {
        if (str::iequals(existing.name, ch.name)) {
            if (existing.dead) {
                log() << "  [cast] " << existing.name
                      << " is dead -- ignoring re-entry\n";
                return existing;
            }
            bool was_off = !existing.on_stage;
            existing.on_stage = true;
            if (existing.description.empty() && !ch.description.empty())
                existing.description = std::move(ch.description);
            if (existing.dialogue_instructions.empty() && !ch.dialogue_instructions.empty())
                existing.dialogue_instructions = std::move(ch.dialogue_instructions);
            if (was_off)
                log() << "  [cast] " << existing.name
                      << " re-enters (was off-stage)\n";
            return existing;
        }
    }
    ch.on_stage = true;
    characters.push_back(std::move(ch));

    auto& added = characters.back();
    log() << "  [cast] NEW " << added.name
          << " | role=" << (added.role.empty() ? "?" : added.role)
          << " | \"" << added.description.substr(0, 80) << "\"\n";

    if (!added.is_player && character_memories.find(added.name) == character_memories.end()) {
        // A newly-entering character starts with an empty mind.  We do NOT copy
        // world-graph facts into it -- that would make every arrival omniscient.
        // Knowledge accrues only through narrator-routed perception.
        CharacterMemory mem(added.name);
        character_memories.emplace(added.name, std::move(mem));
        log() << "  [cast]   memory created (empty -- learns by perception)\n";
    }
    return added;
}

Character* Scene::find_on_stage(const std::string& name) {
    for (auto& c : characters)
        if (c.on_stage && !c.dead && str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* Scene::find_on_stage(const std::string& name) const {
    for (const auto& c : characters)
        if (c.on_stage && !c.dead && str::iequals(c.name, name)) return &c;
    return nullptr;
}

bool Scene::exit_character(const std::string& name) {
    for (auto& c : characters) {
        if (str::iequals(c.name, name) && c.on_stage) {
            c.on_stage = false;
            return true;
        }
    }
    return false;
}

std::vector<DeathCandidate> Scene::scan_death_candidates() {
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

namespace {

std::string join_sorted(const std::vector<std::string>& names) {
    std::vector<std::string> sorted = names;
    std::sort(sorted.begin(), sorted.end());
    std::string out;
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i) out += ", ";
        out += sorted[i];
    }
    return out;
}

}  // namespace

std::vector<std::string> Scene::build_prompt__cast() const {
    std::vector<std::string> lines;
    std::vector<std::string> on_stage_names;
    std::vector<std::string> off_stage_names;

    for (const auto& c : characters) {
        if (c.is_player || c.dead) continue;
        if (c.on_stage) {
            on_stage_names.push_back(c.name);
            std::string line = "- " + c.name;
            if (!c.role.empty())
                line += " [" + c.role + "]";
            const std::string desc = str::trim(c.description);
            if (!desc.empty())
                line += " \xe2\x80\x94" + desc;
            lines.push_back(std::move(line));
        } else {
            off_stage_names.push_back(c.name);
        }
    }

    std::vector<std::string> header;
    if (!on_stage_names.empty())
        header.push_back("On-stage: " + join_sorted(on_stage_names));
    if (!off_stage_names.empty())
        header.push_back("Off-stage: " + join_sorted(off_stage_names));

    header.insert(header.end(), lines.begin(), lines.end());
    return header;
}

std::vector<SceneMessage> Scene::display_timeline(std::optional<size_t> cap) const {
    std::vector<SceneMessage> merged;
    merged.reserve(history.size() + dialogue.size());
    for (const auto& m : history.messages())
        merged.push_back(m);
    for (const auto& m : dialogue.messages())
        merged.push_back(m);
    std::sort(merged.begin(), merged.end(),
              [](const SceneMessage& a, const SceneMessage& b) {
                  return a.timestamp < b.timestamp;
              });
    if (cap.has_value() && merged.size() > *cap)
        return std::vector<SceneMessage>(merged.end() - static_cast<ptrdiff_t>(*cap),
                                          merged.end());
    return merged;
}

// -- Undo --

int Scene::revert_turns(int n) {
    if (n <= 0 || turn_index <= 0) return 0;
    int target = std::max(0, turn_index - n);
    int actual = turn_index - target;

    // 1. History: find the Nth user message from the end, truncate before it
    const auto& msgs = history.messages();
    int user_count = 0;
    size_t cut = msgs.size();
    for (size_t i = msgs.size(); i > 0; --i) {
        if (msgs[i - 1].role == Role::User && ++user_count >= actual) {
            cut = i - 1;
            break;
        }
    }
    history.truncate(cut);
    dialogue.drop_from_turn(target);

    // 2. Graph rollback + ChromaDB cleanup
    auto removed_ids = world_graph.revert_to_turn(target);
    if (memory_) memory_->delete_nodes(removed_ids);

    // 3. Characters: remove spawned-during-reverted-turns, reset + re-evaluate rest
    characters.erase(
        std::remove_if(characters.begin(), characters.end(),
            [target](const Character& c) { return c.created_at >= target; }),
        characters.end());

    for (auto& c : characters) {
        if (c.is_player) continue;
        c.dead = false;
    }

    // 4. Prune character memories for removed characters
    for (auto it = character_memories.begin(); it != character_memories.end(); ) {
        bool exists = std::any_of(characters.begin(), characters.end(),
            [&](const Character& c) { return c.name == it->first; });
        it = exists ? std::next(it) : character_memories.erase(it);
    }

    // 5. Turn index
    turn_index = target;

    log() << "  [undo] Reverted " << actual << " turn(s), now at turn "
          << turn_index << "\n" << std::flush;
    return actual;
}

// -- Scenario-format serialization (unchanged) --

nlohmann::json Scene::to_json() const {
    nlohmann::json j;
    j["title"] = title;
    j["system_prompt"] = system_prompt;
    j["characters"] = characters;
    j["history"] = history;
    return j;
}

Scene Scene::from_json(const nlohmann::json& j) {
    Scene s;
    j.at("title").get_to(s.title);
    j.at("system_prompt").get_to(s.system_prompt);
    j.at("characters").get_to(s.characters);
    if (j.contains("history")) {
        s.history = j.at("history").get<History>();
    }
    if (j.contains("seed_messages")) {
        for (const auto& msg : j.at("seed_messages")) {
            s.history.append(msg.get<SceneMessage>());
        }
    }
    return s;
}

Scene Scene::load_json(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Cannot open scene file: " + path);

    nlohmann::json j;
    in >> j;

    Scene s = Scene::from_json(j);

    s.scene_id = std::filesystem::path(path).stem().string();

    nlohmann::json graph_j;
    graph_j["next_id"] = 1;
    graph_j["nodes"] = j.value("nodes", nlohmann::json::array());
    graph_j["edges"] = j.value("edges", nlohmann::json::array());
    s.world_graph = WorldGraph::from_json(graph_j);

    // Bootstrap CharacterMemory from initial_memory in character definitions
    for (const auto& ch_j : j.value("characters", nlohmann::json::array())) {
        std::string name = ch_j.value("name", "");
        if (name.empty() || ch_j.value("is_player", false)) continue;
        if (!ch_j.contains("initial_memory")) continue;

        CharacterMemory mem(name);
        const auto& im = ch_j["initial_memory"];

        // Authored beliefs are the character's t=0 view, seeded into the
        // subjective belief graph as Active, charged nodes tagged with their
        // subject (the optional `about` field -- a name or list of names).  Each
        // node's id is kept by array index so an authored contradiction
        // ("tension_with": <index>) can be cross-linked as a live tension.
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
            seeded_ids[bi] = mem.seed_belief(bj.value("content", ""), about, 0, w);
        }

        // Authored contradictions: a belief may declare it sits in tension with
        // an earlier belief (by its index in this array).  Cross-link the pair so
        // it surfaces in the rendered Tensions section instead of silently
        // chaining.  Both Thoughts stay live.
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

        // The authored `context` entries are first-person interiority: seed them
        // as charged self-beliefs (subject = the character itself) so they enter
        // the live mind and surface via render_thoughts, rather than a separate
        // self_state field that the narrator no longer reads.
        for (const auto& ctx : im.value("context", nlohmann::json::array()))
            mem.seed_belief(ctx.get<std::string>(), {name}, 0);

        s.character_memories.emplace(name, std::move(mem));
    }

    return s;
}

void Scene::save_json(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open())
        throw std::runtime_error("Cannot write scene file: " + path);
    out << to_json().dump(2);
}

// -- Persistence (game state format) --

std::string Scene::save_path(const std::string& saves_dir) const {
    return saves_dir + "/" + scene_id + ".json";
}

bool Scene::has_save(const std::string& saves_dir) const {
    return std::filesystem::exists(save_path(saves_dir));
}

void Scene::load_save(const std::string& saves_dir) {
    std::ifstream in(save_path(saves_dir));
    if (!in.is_open())
        throw std::runtime_error("No save for scene: " + scene_id);

    nlohmann::json j;
    in >> j;

    turn_index = j.value("turn_index", 0);
    if (j.contains("world_graph")) {
        world_graph = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        world_graph = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    } else {
        throw std::runtime_error("Save is missing world_graph/node_pool data");
    }
    history    = j.at("history").get<History>();
    if (j.contains("dialogue"))
        dialogue = j.at("dialogue").get<History>();
    else
        migrate_character_lines_to_dialogue(history, dialogue);

    if (j.contains("characters") && j["characters"].is_array())
        characters = j["characters"].get<std::vector<Character>>();

    if (memory_)
        memory_->set_next_id(j.value("memory_next_id", 0));

    if (j.contains("character_memories") && j["character_memories"].is_object()) {
        for (auto& [name, cm_j] : j["character_memories"].items())
            character_memories.insert_or_assign(name, CharacterMemory::from_json(cm_j));
    }

    if (j.contains("downsampler"))
        downsampler = TextDownsampler::from_json(j["downsampler"]);
}

void Scene::save(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);

    nlohmann::json j;
    j["scene_id"]       = scene_id;
    j["turn_index"]     = turn_index;
    j["memory_next_id"] = memory_ ? memory_->get_next_id() : 0;
    j["world_graph"]    = world_graph.to_json();
    j["history"]        = history;
    j["dialogue"]       = dialogue;
    j["characters"]     = characters;

    nlohmann::json cm_j;
    for (const auto& [name, mem] : character_memories)
        cm_j[name] = mem.to_json();
    j["character_memories"] = std::move(cm_j);
    j["downsampler"] = downsampler.to_json();

    std::ofstream out(save_path(saves_dir));
    if (!out.is_open())
        throw std::runtime_error("Cannot write save for: " + scene_id);
    out << j.dump(2);
}

void Scene::delete_save(const std::string& saves_dir) const {
    std::filesystem::remove(save_path(saves_dir));
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

// Discover entities that case-insensitively match the query string.
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

// Collect all nodes carrying the given (lowercased) entity, sorted ascending by
// created_at, capped at max_count.
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

// Substring search on node facts, sorted descending by created_at, capped.
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

std::string Scene::tool_query_graph(const std::string& query) const {
    const std::string query_lower = str::to_lower(query);
    // Own the all_nodes snapshot so pointers into it stay valid through the
    // whole function — all_nodes() returns by value, so a temporary would leave
    // dangling pointers after its loop ended.
    auto all = world_graph.all_nodes(true);

    // Try entity match first: if any node has an entity matching the query
    // case-insensitively, build the entity-timeline chain for that entity.
    auto matched_entities = find_matching_entities(all, query);
    if (!matched_entities.empty()) {
        nlohmann::json chains = nlohmann::json::array();
        for (const auto& ent_lower : matched_entities) {
            auto timeline = collect_entity_timeline(all, ent_lower);

            nlohmann::json chain;
            // Find the original-cased entity name from the first matching node
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

    // Fall back to text match: substring search on node facts
    auto matches = find_fact_substring_matches(all, query_lower);
    nlohmann::json ms = nlohmann::json::array();
    for (const auto* n : matches)
        ms.push_back(node_to_json(*n));

    nlohmann::json result;
    result["chains"] = nlohmann::json::array();
    result["matches"] = std::move(ms);
    return result.dump();
}

std::string Scene::tool_query_mind(const std::string& character) const {
    // Find character by case-insensitive name
    const Character* ch = nullptr;
    for (const auto& c : characters) {
        if (str::iequals(c.name, character)) {
            ch = &c;
            break;
        }
    }

    auto it = character_memories.end();
    if (ch) {
        it = character_memories.find(ch->name);
    }
    // Fallback: try case-insensitive key match
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

std::string Scene::tool_query_history(const std::string& query) const {
    const std::string query_lower = str::to_lower(query);

    // Simple keyword split on whitespace
    std::vector<std::string> keywords;
    {
        std::string current;
        for (char c : query_lower) {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!current.empty()) {
                    keywords.push_back(std::move(current));
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty())
            keywords.push_back(std::move(current));
    }

    const auto& messages = history.messages();
    std::vector<const SceneMessage*> matches;
    for (const auto& msg : messages) {
        const std::string text_lower = str::to_lower(msg.content);
        for (const auto& kw : keywords) {
            if (text_lower.find(kw) != std::string::npos) {
                matches.push_back(&msg);
                break;
            }
        }
    }

    // Most recent first, cap at 10
    std::reverse(matches.begin(), matches.end());
    if (matches.size() > 10)
        matches.resize(10);

    auto role_str = [](Role r) -> const char* {
        switch (r) {
            case Role::User:      return "user";
            case Role::Assistant: return "assistant";
            case Role::System:    return "system";
        }
        return "user";
    };

    nlohmann::json snippets = nlohmann::json::array();
    for (const auto* msg : matches) {
        nlohmann::json s;
        s["role"] = role_str(msg->role);
        s["text"] = truncate_utf8(msg->content, 400);
        snippets.push_back(std::move(s));
    }

    nlohmann::json result;
    result["snippets"] = std::move(snippets);
    return result.dump();
}

} // namespace rhapsode
