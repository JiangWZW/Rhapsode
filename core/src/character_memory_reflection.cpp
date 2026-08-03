#include "rhapsode/character_memory.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

struct WeightTuning {
    float reinforce = 1.0f;
    float decay = 0.9f;
    float cull_floor = 0.05f;
    float weight_cap = 10.0f;
    int touch_hops = 3;
};

struct Perception {
    std::uint64_t id = 0;
    std::string fact;
    std::vector<std::string> entities;
};

struct KnowFact {
    std::string fact;
    std::vector<std::string> entities;
    float weight = 5.0f;
    std::string kind = "evidence";  // evidence | tension
};

std::vector<Perception> gather_active_perceptions(const WorldGraph& beliefs) {
    std::vector<Perception> out;
    beliefs.for_each([&](const Node& n) {
        if (n.type != "perception" || n.state != NodeState::Active) return;
        if (n.valid_until != -1) return;
        out.push_back({n.id, n.fact, n.entities});
    }, false);
    return out;
}

void consolidate_perceptions(WorldGraph& beliefs,
                             const std::vector<Perception>& perceptions,
                             int turn) {
    for (const auto& perception : perceptions)
        beliefs.set_valid_until(perception.id, turn);
}

int apply_reinforce_decay(WorldGraph& beliefs, int turn,
                          const std::vector<std::uint64_t>& new_ids,
                          const WeightTuning& tuning,
                          const std::string& character_name) {
    std::unordered_set<std::uint64_t> touched;
    for (const auto new_id : new_ids) {
        touched.insert(new_id);
        for (const auto neighbor_id :
             beliefs.neighbors_within(new_id, tuning.touch_hops)) {
            touched.insert(neighbor_id);
            Node* neighbor = beliefs.get_node(neighbor_id);
            if (neighbor && neighbor->type == "belief") {
                neighbor->weight = std::min(
                    tuning.weight_cap,
                    neighbor->weight + tuning.reinforce);
            }
        }
    }
    std::vector<std::uint64_t> live;
    beliefs.for_each([&](const Node& n) {
        if (n.type == "belief" && n.state == NodeState::Active)
            live.push_back(n.id);
    }, false);
    int culled = 0;
    for (const auto belief_id : live) {
        if (touched.count(belief_id)) continue;
        Node* belief = beliefs.get_node(belief_id);
        if (!belief) continue;
        belief->weight *= tuning.decay;
        if (belief->weight < tuning.cull_floor) {
            beliefs.set_valid_until(belief_id, turn);
            ++culled;
            log() << "  [char_mem:" << character_name << "] culled #"
                  << belief_id << " (w=" << belief->weight << "): "
                  << belief->fact << "\n";
        }
    }
    return culled;
}

std::string extract_json_object(const std::string& raw) {
    const auto start = raw.find('{');
    const auto end = raw.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return {};
    return raw.substr(start, end - start + 1);
}

std::string build_monologue_prompt(
    const std::string& name,
    const CharacterCore& core,
    const std::vector<MonologueStream>& streams,
    const std::string& description,
    const std::string& beat_stimulus,
    const std::vector<Perception>& perceptions,
    const std::string& prior_beliefs) {
    std::ostringstream os;
    os <<
        "You are the actor for ONE character after a public story beat (a \"take\").\n"
        "The narrator already wrote the stage action and spoken lines. You do not.\n"
        "You hold continuity and, when needed, improvise private subtext.\n\n"
        "Layers:\n"
        "- CORE = character bible / continuity sheet (who you are): a deep "
        "durable analysis of identity — NOT a thought stream, NOT first-person "
        "self-talk. Do not append thoughts to core.\n"
        "- STREAMS = the only place for self-aware / in-the-moment interiority. "
        "Focus = objective/through-line. Appends = subtext for this take.\n"
        "- KNOWS = durable subjective facts for the belief graph (what you will still "
        "know later). Empty knows is normal.\n"
        "- Most takes: listen. Empty appends and ops:[] are correct acting.\n\n"
        "Rules:\n"
        "1. No-op is success (background / unpressured / only listening).\n"
        "2. Do not re-narrate the scene. Do not write dialogue.\n"
        "3. Improvise reaction WITHOUT breaking character or inventing other minds.\n"
        "4. Noticing a fact != needing a stream line or a knows entry.\n"
        "5. Fork/merge/conclude shift objectives — prefer ops:[]. Never drop the last "
        "active stream. Max " << CharacterMemory::kMaxActiveStreams << " active.\n"
        "6. Reply with ONLY JSON.\n\n"
        "Character: " << name << "\n";
    if (!description.empty())
        os << "Seed description: " << description << "\n";
    os << "CORE (continuity sheet):\n"
       << (core.text.empty() ? "(empty — may set core_revision once if needed)"
                             : core.text)
       << "\n\nActive streams:\n";
    for (const auto& stream : streams) {
        if (stream.status != "active") continue;
        os << "- id=" << stream.id << " focus=" << stream.focus << "\n";
        const int start = stream.lines.size() > 3
            ? static_cast<int>(stream.lines.size()) - 3 : 0;
        for (int i = start; i < static_cast<int>(stream.lines.size()); ++i)
            os << "    t" << stream.lines[i].turn << ": " << stream.lines[i].text
               << "\n";
    }
    if (!prior_beliefs.empty())
        os << "\nPrior factual beliefs (compact):\n" << prior_beliefs << "\n";
    os << "\nThis take (given circumstances):\n"
       << (beat_stimulus.empty() ? "(none)" : beat_stimulus) << "\n";
    if (!perceptions.empty()) {
        os << "\nRouted perceptions (stimulus only — commit via knows if lasting):\n";
        for (const auto& perception : perceptions)
            os << "- #" << perception.id << " " << perception.fact << "\n";
    }
    os << "\nJSON schema:\n"
          "{\"appends\":[{\"stream_id\":\"...\",\"text\":\"...\"}],"
          "\"ops\":[{\"op\":\"fork\",\"parent\":\"...\",\"focus\":\"...\","
          "\"opening\":\"optional\"}|"
          "{\"op\":\"merge\",\"from\":\"...\",\"into\":\"...\","
          "\"reason\":\"...\",\"synthesis\":\"...\"}|"
          "{\"op\":\"conclude\",\"stream_id\":\"...\",\"reason\":\"...\","
          "\"closure\":\"...\"}],"
          "\"knows\":[{\"fact\":\"...\",\"entities\":[\"Name\"],\"weight\":5,"
          "\"relation\":\"evidence\"}],"
          "\"core_revision\":null}\n";
    return os.str();
}

std::vector<std::uint64_t> apply_knows(
    WorldGraph& beliefs, int turn,
    const std::vector<KnowFact>& knows,
    const std::vector<Perception>& perceptions) {
    std::vector<std::uint64_t> new_ids;
    for (const auto& item : knows) {
        if (item.fact.empty()) continue;
        Node belief;
        belief.fact = sanitize_utf8(item.fact);
        belief.type = "belief";
        belief.state = NodeState::Active;
        belief.entities = item.entities;
        belief.created_at = turn;
        belief.valid_until = -1;
        belief.weight = std::clamp(item.weight, 1.0f, 10.0f);
        const std::uint64_t new_id = beliefs.add_node(std::move(belief)).id;
        new_ids.push_back(new_id);
        for (const auto& perception : perceptions) {
            bool overlap = item.entities.empty();
            for (const auto& entity : item.entities) {
                for (const auto& pe : perception.entities) {
                    if (str::iequals(entity, pe)) overlap = true;
                }
            }
            if (overlap || item.entities.empty())
                beliefs.add_relation(
                    new_id, perception.id, 1.0f, turn, "evidence");
        }
    }
    return new_ids;
}

}  // namespace

void CharacterMemory::ensure_bootstrap(const std::string& core_text_if_empty) {
    if (core_.text.empty() && !core_text_if_empty.empty())
        core_.text = sanitize_utf8(core_text_if_empty);
    if (active_stream_count() > 0) return;
    MonologueStream stream;
    stream.id = "self";
    stream.focus = "ambient self";
    stream.status = "active";
    streams_.push_back(std::move(stream));
}

int CharacterMemory::active_stream_count() const {
    int count = 0;
    for (const auto& stream : streams_)
        if (stream.status == "active") ++count;
    return count;
}

MonologueStream* CharacterMemory::find_stream(const std::string& id) {
    for (auto& stream : streams_)
        if (stream.id == id) return &stream;
    return nullptr;
}

const MonologueStream* CharacterMemory::find_stream(const std::string& id) const {
    for (const auto& stream : streams_)
        if (stream.id == id) return &stream;
    return nullptr;
}

std::string CharacterMemory::alloc_stream_id(const std::string& parent_id,
                                            int turn) {
    ++stream_seq_;
    return parent_id + "_f" + std::to_string(turn) + "_" +
           std::to_string(stream_seq_);
}

nlohmann::json CharacterMemory::render_mind_query(
    std::size_t max_belief_chars, std::size_t max_line_chars) const {
    nlohmann::json result;
    result["core"] = sanitize_utf8(
        truncate_utf8(core_.text, static_cast<int>(max_belief_chars / 2)));
    nlohmann::json active = nlohmann::json::array();
    for (const auto& stream : streams_) {
        if (stream.status != "active") continue;
        nlohmann::json row;
        row["id"] = stream.id;
        row["focus"] = stream.focus;
        nlohmann::json lines = nlohmann::json::array();
        const int start = stream.lines.size() > 3
            ? static_cast<int>(stream.lines.size()) - 3 : 0;
        for (int i = start; i < static_cast<int>(stream.lines.size()); ++i) {
            nlohmann::json line;
            line["turn"] = stream.lines[i].turn;
            line["text"] = sanitize_utf8(truncate_utf8(
                stream.lines[i].text, static_cast<int>(max_line_chars)));
            lines.push_back(std::move(line));
        }
        row["recent_lines"] = std::move(lines);
        active.push_back(std::move(row));
    }
    result["streams"] = std::move(active);
    result["beliefs"] = sanitize_utf8(
        truncate_utf8(render_thoughts({}), static_cast<int>(max_belief_chars)));
    return result;
}

void CharacterMemory::update_monologues(
    int turn,
    const std::string& description,
    const std::string& beat_stimulus,
    const LLMCallback& llm_callback) {
    ensure_bootstrap(description);
    if (!llm_callback) {
        log() << "  [char_mem:" << character_name_
              << "] monologue skip: no LLM callback\n" << std::flush;
        return;
    }

    auto perceptions = gather_active_perceptions(beliefs_);
    const std::string prior = truncate_utf8(render_thoughts({}), 800);
    const std::string prompt = build_monologue_prompt(
        character_name_, core_, streams_, description, beat_stimulus,
        perceptions, prior);

    std::string raw;
    try {
        raw = llm_callback(prompt);
    } catch (const std::exception& ex) {
        log() << "  [char_mem:" << character_name_
              << "] update_monologues failed: " << ex.what() << "\n";
        consolidate_perceptions(beliefs_, perceptions, turn);
        return;
    }

    nlohmann::json parsed = try_parse_json(raw);
    if (parsed.is_null() || !parsed.is_object()) {
        const auto sliced = extract_json_object(raw);
        if (!sliced.empty()) parsed = try_parse_json(sliced);
    }

    std::vector<std::uint64_t> new_belief_ids;

    if (parsed.is_object()) {
        if (parsed.contains("core_revision") && parsed["core_revision"].is_string()) {
            const std::string revision =
                str::trim(sanitize_utf8(parsed["core_revision"].get<std::string>()));
            if (!revision.empty()) {
                core_.text = revision;
                core_.revised_at = turn;
            }
        }

        // merges then concludes then forks
        if (parsed.contains("ops") && parsed["ops"].is_array()) {
            std::vector<nlohmann::json> merges, concludes, forks;
            for (const auto& op : parsed["ops"]) {
                if (!op.is_object()) continue;
                const std::string kind = str::to_lower(op.value("op", ""));
                if (kind == "merge") merges.push_back(op);
                else if (kind == "conclude") concludes.push_back(op);
                else if (kind == "fork") forks.push_back(op);
            }
            for (const auto& op : merges) {
                const std::string from_id = op.value("from", "");
                const std::string into_id = op.value("into", "");
                auto* from = find_stream(from_id);
                auto* into = find_stream(into_id);
                if (!from || !into || from == into) continue;
                if (from->status != "active" || into->status != "active") continue;
                const std::string synthesis =
                    str::trim(sanitize_utf8(op.value("synthesis", "")));
                if (!synthesis.empty())
                    into->lines.push_back({turn, synthesis});
                from->status = "closed";
                from->closed_reason = str::trim(op.value("reason", "merged"));
                from->closed_summary = synthesis.empty()
                    ? from->closed_reason : synthesis;
                log() << "  [char_mem:" << character_name_
                      << "] stream merge " << from_id << " -> " << into_id << "\n";
            }
            for (const auto& op : concludes) {
                const std::string stream_id = op.value("stream_id", "");
                auto* stream = find_stream(stream_id);
                if (!stream || stream->status != "active") continue;
                if (active_stream_count() <= 1) {
                    log() << "  [char_mem:" << character_name_
                          << "] skip conclude " << stream_id
                          << " (would empty streams)\n";
                    continue;
                }
                stream->status = "closed";
                stream->closed_reason = str::trim(op.value("reason", "concluded"));
                stream->closed_summary =
                    str::trim(sanitize_utf8(op.value("closure", "")));
                log() << "  [char_mem:" << character_name_
                      << "] stream conclude " << stream_id << "\n";
            }
            for (const auto& op : forks) {
                if (active_stream_count() >= kMaxActiveStreams) {
                    log() << "  [char_mem:" << character_name_
                          << "] skip fork (at cap)\n";
                    continue;
                }
                const std::string parent_id = op.value("parent", "");
                auto* parent = find_stream(parent_id);
                if (!parent || parent->status != "active") continue;
                const std::string focus =
                    str::trim(sanitize_utf8(op.value("focus", "")));
                if (focus.empty()) continue;
                MonologueStream child;
                child.id = alloc_stream_id(parent_id, turn);
                child.focus = focus;
                child.parent_id = parent_id;
                child.status = "active";
                const std::string opening =
                    str::trim(sanitize_utf8(op.value("opening", "")));
                if (!opening.empty())
                    child.lines.push_back({turn, opening});
                log() << "  [char_mem:" << character_name_
                      << "] stream fork " << parent_id << " -> " << child.id
                      << "\n";
                streams_.push_back(std::move(child));
            }
        }

        if (parsed.contains("appends") && parsed["appends"].is_array()) {
            for (const auto& append : parsed["appends"]) {
                if (!append.is_object()) continue;
                const std::string stream_id = append.value("stream_id", "");
                const std::string text =
                    str::trim(sanitize_utf8(append.value("text", "")));
                if (text.empty()) continue;
                auto* stream = find_stream(stream_id);
                if (!stream || stream->status != "active") continue;
                stream->lines.push_back({turn, text});
            }
        }

        if (parsed.contains("knows") && parsed["knows"].is_array()) {
            std::vector<KnowFact> knows;
            for (const auto& value : parsed["knows"]) {
                if (!value.is_object()) continue;
                KnowFact item;
                item.fact = str::trim(sanitize_utf8(value.value("fact", "")));
                if (item.fact.empty()) continue;
                if (value.contains("entities") && value["entities"].is_array()) {
                    for (const auto& entity : value["entities"])
                        if (entity.is_string())
                            item.entities.push_back(entity.get<std::string>());
                }
                item.weight = static_cast<float>(
                    std::clamp(json_number<int>(value, "weight", 5), 1, 10));
                const std::string relation =
                    str::to_lower(value.value("relation", "evidence"));
                item.kind =
                    (relation.find("tension") != std::string::npos ||
                     relation.find("contradict") != std::string::npos)
                        ? "tension" : "evidence";
                knows.push_back(std::move(item));
            }
            new_belief_ids = apply_knows(beliefs_, turn, knows, perceptions);
            // tension: link new knows that requested tension against newest prior
            // (simple: skip cross-link complexity; evidence edges to perceptions done)
        }
    } else {
        log() << "  [char_mem:" << character_name_
              << "] monologue unparseable -- no-op apply\n";
    }

    consolidate_perceptions(beliefs_, perceptions, turn);
    apply_reinforce_decay(
        beliefs_, turn, new_belief_ids, WeightTuning{}, character_name_);
    ensure_bootstrap(description);

    log() << "  [char_mem:" << character_name_ << "] monologue done appends/ops "
          << "knows=" << new_belief_ids.size()
          << " streams_active=" << active_stream_count() << "\n" << std::flush;
}

}  // namespace rhapsode
