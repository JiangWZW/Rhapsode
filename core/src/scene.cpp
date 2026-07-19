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

// -- Storyline lifecycle --

Scene Scene::fork(const std::string& new_scene_id,
                  const std::vector<std::string>& cast) const {
    Scene child;
    child.set_world(world_);        // co-own the same durable substrate
    child.scene_id      = new_scene_id;
    child.title         = title;    // inherited; caller may override
    child.system_prompt = system_prompt;
    child.turn_index    = 0;        // fresh clock; child has its own prose threads

    // A fork MOVES its cast: they join the child and leave this parent, so a
    // character is never in both storylines at once. The player never moves.
    for (const auto& name : cast)
        if (Character* ch = world_->find_character(name); ch && !ch->is_player) {
            ch->join_scene(new_scene_id);
            ch->leave_scene(scene_id);
        }

    log() << "  [fork] scene '" << scene_id << "' -> '" << new_scene_id
          << "' with cast [" << [&] {
                 std::string s;
                 for (size_t i = 0; i < cast.size(); ++i) {
                     if (i) s += ", ";
                     s += cast[i];
                 }
                 return s;
             }() << "]\n";
    return child;
}

// -- Character lifecycle (theatre model) --

Character& Scene::enter_character(Character ch) {
    auto& characters = world_->characters;
    auto& character_memories = world_->character_memories;
    for (auto& existing : characters) {
        if (str::iequals(existing.name, ch.name)) {
            if (existing.dead) {
                log() << "  [cast] " << existing.name
                      << " is dead -- ignoring re-entry\n";
                return existing;
            }
            bool was_off = !existing.in_scene(scene_id);
            existing.join_scene(scene_id);
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
    ch.join_scene(scene_id);
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
    for (auto& c : world_->characters)
        if (c.in_scene(scene_id) && !c.dead && str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* Scene::find_on_stage(const std::string& name) const {
    for (const auto& c : world_->characters)
        if (c.in_scene(scene_id) && !c.dead && str::iequals(c.name, name)) return &c;
    return nullptr;
}

bool Scene::exit_character(const std::string& name) {
    for (auto& c : world_->characters) {
        if (str::iequals(c.name, name) && c.in_scene(scene_id)) {
            c.leave_scene(scene_id);
            return true;
        }
    }
    return false;
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

    for (const auto& c : world_->characters) {
        if (c.is_player || c.dead) continue;
        if (c.in_scene(scene_id)) {
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

    auto& world_graph = world_->world_graph;
    auto& characters = world_->characters;
    auto& character_memories = world_->character_memories;

    // 2. Graph rollback + ChromaDB cleanup
    auto removed_ids = world_graph.revert_to_turn(target);
    if (MemorySystem* mem = world_->memory()) mem->delete_nodes(removed_ids);

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
    j["characters"] = world_->characters;
    j["history"] = history;
    return j;
}

Scene Scene::from_json(const nlohmann::json& j) {
    Scene s;
    j.at("title").get_to(s.title);
    j.at("system_prompt").get_to(s.system_prompt);
    j.at("characters").get_to(s.world().characters);
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

    // Populate the shared substrate (graph + authored minds) from the scenario.
    s.world().seed_from_scenario(j);

    // Authored scenarios express initial presence with an `on_stage` bool (a
    // per-scene convenience). Now that the scene id is known, resolve that hint
    // into explicit membership so the character sits in this storyline.
    for (const auto& ch_j : j.value("characters", nlohmann::json::array())) {
        std::string name = ch_j.value("name", "");
        if (name.empty()) continue;
        bool authored_on = ch_j.value("on_stage", ch_j.value("is_player", false));
        if (!authored_on) continue;
        for (auto& c : s.world().characters)
            if (str::iequals(c.name, name)) { c.join_scene(s.scene_id); break; }
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

bool Scene::has_ephemeral_save(const std::string& saves_dir) const {
    return std::filesystem::exists(save_path(saves_dir));
}

bool Scene::has_save(const std::string& saves_dir) const {
    // Require both blobs: a legacy single-file save (no world.json) is treated
    // as absent so the session starts fresh instead of failing to load.
    return world_->has_save(saves_dir) && has_ephemeral_save(saves_dir);
}

void Scene::load_ephemeral(const std::string& saves_dir) {
    std::ifstream in(save_path(saves_dir));
    if (!in.is_open())
        throw std::runtime_error("No save for scene: " + scene_id);

    nlohmann::json j;
    in >> j;

    turn_index        = j.value("turn_index", 0);
    driving_intention = j.value("driving_intention", std::string{});
    charge            = j.value("charge", 0.0f);
    last_advanced     = j.value("last_advanced", 0);
    if (title.empty())         title = j.value("title", std::string{});
    if (system_prompt.empty()) system_prompt = j.value("system_prompt", std::string{});

    history = j.at("history").get<History>();
    if (j.contains("dialogue"))
        dialogue = j.at("dialogue").get<History>();
    else
        migrate_character_lines_to_dialogue(history, dialogue);

    if (j.contains("downsampler"))
        downsampler = TextDownsampler::from_json(j["downsampler"]);
}

void Scene::save_ephemeral(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);

    nlohmann::json j;
    j["scene_id"]          = scene_id;
    j["title"]             = title;
    j["system_prompt"]     = system_prompt;
    j["turn_index"]        = turn_index;
    j["driving_intention"] = driving_intention;
    j["charge"]            = charge;
    j["last_advanced"]     = last_advanced;
    j["history"]           = history;
    j["dialogue"]          = dialogue;
    j["downsampler"]       = downsampler.to_json();

    std::ofstream out(save_path(saves_dir));
    if (!out.is_open())
        throw std::runtime_error("Cannot write save for: " + scene_id);
    out << j.dump(2);
}

void Scene::load_save(const std::string& saves_dir) {
    // Durable substrate first (graph, roster, minds, memory id), then this
    // scene's ephemeral blob.
    world_->load_save(saves_dir);
    load_ephemeral(saves_dir);
}

void Scene::save(const std::string& saves_dir) const {
    // Durable substrate -> world.json (shared), then this scene's blob.
    world_->save(saves_dir);
    save_ephemeral(saves_dir);
}

void Scene::delete_save(const std::string& saves_dir) const {
    world_->delete_save(saves_dir);
    std::filesystem::remove(save_path(saves_dir));
}

// -- Narrator tool-use queries -----------------------------------------------
// query_graph / query_mind live on World (shared substrate). query_history is
// per-scene: it searches this storyline's prose thread.

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
