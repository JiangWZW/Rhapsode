#include "rhapsode/scene.h"
#include "rhapsode/memory_system.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace rhapsode {

namespace {

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin(),
        [](char ca, char cb) {
            return std::tolower(static_cast<unsigned char>(ca)) ==
                   std::tolower(static_cast<unsigned char>(cb));
        });
}

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
        if (iequals(existing.name, ch.name)) {
            if (existing.dead) {
                std::cerr << "  [cast] " << existing.name
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
                std::cerr << "  [cast] " << existing.name
                          << " re-enters (was off-stage)\n";
            return existing;
        }
    }
    ch.on_stage = true;
    characters.push_back(std::move(ch));

    auto& added = characters.back();
    std::cerr << "  [cast] NEW " << added.name
              << " | role=" << (added.role.empty() ? "?" : added.role)
              << " | \"" << added.description.substr(0, 80) << "\"\n";

    if (!added.is_player && character_memories.find(added.name) == character_memories.end()) {
        // A newly-entering character starts with an empty mind.  We do NOT copy
        // world-graph facts into it -- that would make every arrival omniscient.
        // Knowledge accrues only through narrator-routed perception.
        CharacterMemory mem(added.name);
        mem.set_persona(added.description);
        character_memories.emplace(added.name, std::move(mem));
        std::cerr << "  [cast]   memory created (empty -- learns by perception)\n";
    }
    return added;
}

Character* Scene::find_on_stage(const std::string& name) {
    for (auto& c : characters)
        if (c.on_stage && !c.dead && iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* Scene::find_on_stage(const std::string& name) const {
    for (const auto& c : characters)
        if (c.on_stage && !c.dead && iequals(c.name, name)) return &c;
    return nullptr;
}

bool Scene::exit_character(const std::string& name) {
    for (auto& c : characters) {
        if (iequals(c.name, name) && c.on_stage) {
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

    auto to_lower_copy = [](const std::string& s) {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    };

    auto has_death_keyword = [&](const std::string& text_lower) {
        for (const auto& kw : death_keywords)
            if (text_lower.find(kw) != std::string::npos) return true;
        return false;
    };

    auto mentions_character = [&](const Node& n, const std::string& name,
                                  const std::string& name_lower) {
        for (const auto& ent : n.entities)
            if (iequals(ent, name)) return true;
        auto fl = to_lower_copy(n.fact);
        if (fl.find(name_lower) != std::string::npos) return true;
        auto cl = to_lower_copy(n.active_ctx);
        return cl.find(name_lower) != std::string::npos;
    };

    for (const auto& ch : characters) {
        if (ch.is_player || ch.dead) continue;

        std::string name_lower = to_lower_copy(ch.name);
        bool has_death_hit = false;

        for (const auto& n : all_nodes) {
            if (n.state != NodeState::Active) continue;
            if (!mentions_character(n, ch.name, name_lower)) continue;

            std::string fact_lower = to_lower_copy(n.fact);
            std::string ctx_lower  = to_lower_copy(n.active_ctx);
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

std::string trim_copy(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

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
            const std::string desc = trim_copy(c.description);
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

std::vector<std::string> Scene::thought_subjects_for(const Character& observer) const
{
    std::vector<std::string> subjects;
    for (const auto& c : characters) 
    {
        if (c.name == observer.name || c.dead || !c.on_stage) 
            continue;

    	subjects.push_back(c.name);
        if (c.is_player && !c.description.empty())
            subjects.push_back(c.description);
    }
    subjects.push_back(observer.name);
    return subjects;
}

std::string Scene::build_prompt__inner_lives(int turn) const
{
    std::string body;
    for (const auto& ch : characters) 
    {
        if (ch.is_player || ch.dead || !ch.on_stage) 
            continue;

    	auto it = character_memories.find(ch.name);
        if (it == character_memories.end()) 
            continue;

        std::string block = ch.build_prompt__dialogue_voice();
        block += it->second.build_prompt__interior_thoughts(thought_subjects_for(ch), turn);
        if (block.empty()) continue;
        body += "- " + ch.name + ":\n" + block;
    }
    if (body.empty()) return {};

    return "### Inner lives\n"
           "(Each character's voice and the interior you are writing FROM. These "
           "are not commands -- perform them. A high-weight thought in tension "
           "pulls toward subtext, a slip, or crisis; a high-weight thought "
           "standing alone pulls toward action. You decide what surfaces.)\n"
           + body;
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

    std::cerr << "  [undo] Reverted " << actual << " turn(s), now at turn "
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
        mem.set_persona(ch_j.value("description", ""));
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

    // Persona is not serialized -- re-attach it from each Character's description
    // (single source of truth) so first-person prompts keep the right identity.
    for (const auto& ch : characters) {
        auto it = character_memories.find(ch.name);
        if (it != character_memories.end())
            it->second.set_persona(ch.description);
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

} // namespace rhapsode
