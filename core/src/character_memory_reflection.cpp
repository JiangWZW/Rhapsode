#include "rhapsode/character_memory.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>
#include <vector>

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

struct KnowFact {
    std::string fact;
    std::vector<std::string> entities;
    float weight = 5.0f;
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

constexpr char kMonologueUserSentinel[] = "<<<RHAPSODE_MONOLOGUE_USER>>>";

constexpr char kMonologueJsonSchema[] =
    "{\"appends\":[{\"stream_id\":\"...\",\"text\":\"...\"}],"
    "\"ops\":[{\"op\":\"fork\",\"parent\":\"...\",\"focus\":\"...\","
    "\"opening\":\"optional\"}|"
    "{\"op\":\"merge\",\"from\":\"...\",\"into\":\"...\","
    "\"reason\":\"...\",\"synthesis\":\"...\"}|"
    "{\"op\":\"conclude\",\"stream_id\":\"...\",\"reason\":\"...\","
    "\"closure\":\"...\"}],"
    "\"knows\":[{\"fact\":\"...\",\"entities\":[\"Name\"],\"weight\":5,"
    "\"relation\":\"evidence\"}],"
    "\"core_revision\":null}";

std::string monologue_system_instructions() {
    std::ostringstream os;
    os <<
        "You are playing one human being, from the inside, in the silence after a take.\n"
        "The public scene is already written. You do not speak. You do not narrate.\n"
        "You do not invent another person's private mind.\n\n"
        "Most takes you only listen. That is good acting.\n"
        "When something lands, it is yours: a private beat of subtext, a shift in a\n"
        "focus, or a fact you will still know tomorrow.\n\n"
        "Live in first person. Do not describe the role from outside.\n"
        "Do not rewrite who you are unless the soul of the role actually changed.\n"
        "Fork/merge/conclude only when a focus or through-line itself splits, joins,\n"
        "or ends. Never drop the last active through-line. Max "
        << CharacterMemory::kMaxActiveStreams << " active.\n"
        "Ids under On your mind are the only ids to cite. What you've been thinking\n"
        "is this mind's private journal, oldest first. What just happened is this\n"
        "take and what you took in.\n\n"
        "Answer with ONLY the JSON sidecar (no prose wrapper). Empty appends and ops "
        "are a listening take.\n\n"
        "JSON:\n"
        << kMonologueJsonSchema << "\n";
    return os.str();
}

std::string build_monologue_user_prompt(
    const std::string& name,
    const CharacterCore& core,
    const std::vector<MonologueStream>& streams,
    const std::string& turn_stimulus) {
    std::ostringstream os;
    os << "You are " << name << ".\n";
    os << "\nWho you are:\n"
       << (core.text.empty()
               ? "(empty — you may set core_revision once if needed)"
               : core.text)
       << "\n";

    os << "\nOn your mind:\n";
    bool any_stream = false;
    for (const auto& stream : streams) {
        if (stream.status != "active") continue;
        any_stream = true;
        os << "- " << stream.id << ": " << stream.focus << "\n";
    }
    if (!any_stream)
        os << "- (none)\n";

    os << "\nWhat you've been thinking:\n";
    struct JournalLine {
        int seq = 0;
        std::string id;
        std::string text;
    };
    std::vector<JournalLine> journal;
    for (const auto& stream : streams) {
        for (const auto& line : stream.lines)
            journal.push_back({line.seq, stream.id, line.text});
    }
    std::sort(journal.begin(), journal.end(),
              [](const JournalLine& a, const JournalLine& b) {
                  return a.seq < b.seq;
              });
    for (const auto& line : journal)
        os << "[" << line.id << "] " << line.text << "\n";

    os << "What just happened:\n"
       << (turn_stimulus.empty() ? "(none)\n" : turn_stimulus + "\n");
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

constexpr char kObservationUserSentinel[] = "<<<RHAPSODE_OBSERVATION_USER>>>";

constexpr char kObservationJsonSchema[] =
    "{\"lines\":[\"...\"],\"facts\":[{\"fact\":\"...\",\"entities\":[]}]}";

std::string observation_system_instructions() {
    return
        "Record what this person could take in from the latest take.\n"
        "You already have who they are and their journal. The latest take is "
        "already on the journal.\n"
        "Write only what they could take in. Skip the rest. Do not copy the "
        "take. Empty lines is fine.\n"
        "Do not invent another person's private mind. No inventory.\n\n"
        "Answer with ONLY the JSON (no prose wrapper).\n\n"
        "JSON:\n" + std::string(kObservationJsonSchema) + "\n";
}

std::string build_observation_user_payload(
    const std::string& name,
    const std::string& who,
    const std::vector<ObjectiveLine>& journal) {
    std::ostringstream os;
    os << name << "\n\n";
    os << (who.empty() ? "(unknown)" : who) << "\n";
    for (const auto& line : journal) {
        os << "\n[" << line.turn << " " << line.type << "]\n"
           << line.text << "\n";
    }
    return os.str();
}

}  // namespace

std::string CharacterMemory::build_observation_prompt() const
{
    const std::string who_text = !core_.text.empty() ? core_.text : std::string{};
    return observation_system_instructions() + kObservationUserSentinel + "\n" +
           build_observation_user_payload(character_name_, who_text, objective_journal_);
}

void CharacterMemory::apply_observation_json(int turn, const std::string &raw) {
    nlohmann::json parsed = try_parse_json(raw);
    if (parsed.is_null() || !parsed.is_object()) {
        const auto sliced = extract_json_object(raw);
        if (!sliced.empty())
            parsed = try_parse_json(sliced);
    }
    if (!parsed.is_object()) {
        log() << "  [char_mem:" << character_name_ << "] observation unparseable -- no-op apply\n";
        return;
    }
    if (parsed.contains("lines") && parsed["lines"].is_array()) {
        for (const auto &value : parsed["lines"]) {
            if (value.is_string())
                append_objective(turn, "seen", value.get<std::string>());
        }
    }
    if (parsed.contains("facts") && parsed["facts"].is_array()) {
        std::vector<KnowFact> knows;
        for (const auto &value : parsed["facts"]) {
            if (!value.is_object())
                continue;
            KnowFact item;
            item.fact = str::trim(sanitize_utf8(value.value("fact", "")));
            if (item.fact.empty())
                continue;
            if (value.contains("entities") && value["entities"].is_array()) {
                for (const auto &entity : value["entities"])
                    if (entity.is_string())
                        item.entities.push_back(entity.get<std::string>());
            }
            knows.push_back(std::move(item));
        }
        apply_knows(beliefs_, turn, knows, {});
    }
    int seen_this_turn = 0;
    for (const auto &line : objective_journal_)
        if (line.turn == turn && line.type == "seen")
            ++seen_this_turn;
    log() << "  [journal:" << character_name_ << "] observation done t=" << turn
          << " seen=" << seen_this_turn << " journal=" << objective_journal_.size() << "\n"
          << std::flush;
}

void CharacterMemory::update_objective_journal(int turn, const std::string &who,
                                               const LLMCallback &llm_callback) {
    ensure_bootstrap(who);
    if (!llm_callback) {
        log() << "  [char_mem:" << character_name_ << "] observation skip: no LLM callback\n"
            << std::flush;
        return;
    }
    const std::string prompt = build_observation_prompt();
    std::string raw;
    try {
        raw = llm_callback(prompt);
    } catch (const std::exception &ex) {
        log() << "  [char_mem:" << character_name_
            << "] update_objective_journal failed: " << ex.what() << "\n";
        return;
    }
    apply_observation_json(turn, raw);
}

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

std::string CharacterMemory::alloc_stream_id(const std::string& parent_id,
                                            int turn) {
    ++monologue_stream_cnt_;
    return parent_id + "_f" + std::to_string(turn) + "_" +
           std::to_string(monologue_stream_cnt_);
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

CharacterMemory::MonologueBuildContext
CharacterMemory::build_monologue_prompt(const std::string &description,
                                                  const std::string &turn_stimulus) {
    MonologueBuildContext ctx;
    ctx.description = description;
    ensure_bootstrap(description);
    ctx.perceptions = gather_active_perceptions(beliefs_);
    ctx.prompt = monologue_system_instructions() + kMonologueUserSentinel + "\n"
                + build_monologue_user_prompt(character_name_, core_, streams_, turn_stimulus);
    return ctx;
}
void CharacterMemory::apply_monologue_json(int turn, const std::string &raw,
                                           const MonologueBuildContext &ctx) {
    auto perceptions = ctx.perceptions;
    std::vector<std::uint64_t> new_belief_ids;
    nlohmann::json parsed = try_parse_json(raw);
    if (parsed.is_null() || !parsed.is_object()) {
        const auto sliced = extract_json_object(raw);
        if (!sliced.empty())
            parsed = try_parse_json(sliced);
    }
    if (parsed.is_object()) {
        if (parsed.contains("core_revision") && parsed["core_revision"].is_string()) {
            const std::string revision =
                str::trim(sanitize_utf8(parsed["core_revision"].get<std::string>()));
            if (!revision.empty()) {
                core_.text = revision;
                core_.revised_at = turn;
            }
        }
        if (parsed.contains("ops") && parsed["ops"].is_array()) {
            std::vector<nlohmann::json> merges, concludes, forks;
            for (const auto &op : parsed["ops"]) {
                if (!op.is_object())
                    continue;
                const std::string kind = str::to_lower(op.value("op", ""));
                if (kind == "merge")
                    merges.push_back(op);
                else if (kind == "conclude")
                    concludes.push_back(op);
                else if (kind == "fork")
                    forks.push_back(op);
            }
            for (const auto &op : merges) {
                const std::string from_id = op.value("from", "");
                const std::string into_id = op.value("into", "");
                auto *from = find_stream(from_id);
                auto *into = find_stream(into_id);
                if (!from || !into || from == into)
                    continue;
                if (from->status != "active" || into->status != "active")
                    continue;
                const std::string synthesis = str::trim(sanitize_utf8(op.value("synthesis", "")));
                if (!synthesis.empty())
                    append_monologue_line(*into, turn, synthesis);
                from->status = "closed";
                from->closed_reason = str::trim(op.value("reason", "merged"));
                from->closed_summary = synthesis.empty() ? from->closed_reason : synthesis;
                log() << "  [char_mem:" << character_name_ << "] stream merge " << from_id << " -> "
                      << into_id << "\n";
            }
            for (const auto &op : concludes) {
                const std::string stream_id = op.value("stream_id", "");
                auto *stream = find_stream(stream_id);
                if (!stream || stream->status != "active")
                    continue;
                if (active_stream_count() <= 1) {
                    log() << "  [char_mem:" << character_name_ << "] skip conclude " << stream_id
                          << " (would empty streams)\n";
                    continue;
                }
                stream->status = "closed";
                stream->closed_reason = str::trim(op.value("reason", "concluded"));
                stream->closed_summary = str::trim(sanitize_utf8(op.value("closure", "")));
                log() << "  [char_mem:" << character_name_ << "] stream conclude " << stream_id
                      << "\n";
            }
            for (const auto &op : forks) {
                if (active_stream_count() >= kMaxActiveStreams) {
                    log() << "  [char_mem:" << character_name_ << "] skip fork (at cap)\n";
                    continue;
                }
                const std::string parent_id = op.value("parent", "");
                auto *parent = find_stream(parent_id);
                if (!parent || parent->status != "active")
                    continue;
                const std::string focus = str::trim(sanitize_utf8(op.value("focus", "")));
                if (focus.empty())
                    continue;
                MonologueStream child;
                child.id = alloc_stream_id(parent_id, turn);
                child.focus = focus;
                child.parent_id = parent_id;
                child.status = "active";
                const std::string opening = str::trim(sanitize_utf8(op.value("opening", "")));
                if (!opening.empty())
                    append_monologue_line(child, turn, opening);
                log() << "  [char_mem:" << character_name_ << "] stream fork " << parent_id
                      << " -> " << child.id << "\n";
                streams_.push_back(std::move(child));
            }
        }
        if (parsed.contains("appends") && parsed["appends"].is_array()) {
            for (const auto &append : parsed["appends"]) {
                if (!append.is_object())
                    continue;
                const std::string stream_id = append.value("stream_id", "");
                const std::string text = str::trim(sanitize_utf8(append.value("text", "")));
                if (text.empty())
                    continue;
                auto *stream = find_stream(stream_id);
                if (!stream || stream->status != "active")
                    continue;
                append_monologue_line(*stream, turn, text);
            }
        }
        if (parsed.contains("knows") && parsed["knows"].is_array()) {
            std::vector<KnowFact> knows;
            for (const auto &value : parsed["knows"]) {
                if (!value.is_object())
                    continue;
                KnowFact item;
                item.fact = str::trim(sanitize_utf8(value.value("fact", "")));
                if (item.fact.empty())
                    continue;
                if (value.contains("entities") && value["entities"].is_array()) {
                    for (const auto &entity : value["entities"])
                        if (entity.is_string())
                            item.entities.push_back(entity.get<std::string>());
                }
                item.weight =
                    static_cast<float>(std::clamp(json_number<int>(value, "weight", 5), 1, 10));
                knows.push_back(std::move(item));
            }
            new_belief_ids = apply_knows(beliefs_, turn, knows, perceptions);
        }
    } else {
        log() << "  [char_mem:" << character_name_ << "] monologue unparseable -- no-op apply\n";
    }
    consolidate_perceptions(beliefs_, perceptions, turn);
    apply_reinforce_decay(beliefs_, turn, new_belief_ids, WeightTuning{}, character_name_);
    ensure_bootstrap(ctx.description);
}
void CharacterMemory::update_monologues(int turn, const std::string &description,
                                        const std::string &turn_stimulus,
                                        const LLMCallback &llm_callback, const std::string &voice) {
    (void)voice;
    auto ctx = build_monologue_prompt(description, turn_stimulus);
    if (!llm_callback) {
        log() << "  [char_mem:" << character_name_ << "] monologue skip: no LLM callback\n"
              << std::flush;
        return;
    }
    std::string raw;
    try {
        raw = llm_callback(ctx.prompt);
    } catch (const std::exception &ex) {
        log() << "  [char_mem:" << character_name_ << "] update_monologues failed: " << ex.what()
              << "\n";
    }
    apply_monologue_json(turn, raw, ctx);
}

}  // namespace rhapsode
