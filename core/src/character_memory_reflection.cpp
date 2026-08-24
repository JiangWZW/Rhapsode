#include "rhapsode/character_memory.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

struct KnowFact {
    std::string fact;
    std::vector<std::string> entities;
    float weight = 5.0f;
};

std::string extract_json_object(const std::string& raw) {
    const auto start = raw.find('{');
    const auto end = raw.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return {};
    return raw.substr(start, end - start + 1);
}

void apply_knows(WorldGraph& beliefs, int turn,
                 const std::vector<KnowFact>& knows) {
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
        beliefs.add_node(std::move(belief));
    }
}

constexpr char kMonologueUserSentinel[] = "<<<RHAPSODE_MONOLOGUE_USER>>>";

constexpr char kMonologueJsonSchema[] =
    "{\"line\":\"...\"} or {\"line\":null}";

std::string monologue_system_instructions() {
    return
        "You are this person, from the inside, after a public beat.\n"
        "The public scene is already written. You do not speak. You do not narrate.\n"
        "You do not invent another person's private mind.\n\n"
        "Your private lines are oldest first. After them is what you perceived "
        "from the last three turns.\n"
        "Most beats you only listen. That is good acting.\n"
        "When something lands, write one private first-person line.\n\n"
        "Answer with ONLY the JSON sidecar (no prose wrapper).\n\n"
        "JSON:\n"
        + std::string(kMonologueJsonSchema) + "\n";
}

void emit_piece(std::ostringstream& os, const std::string& text) {
    if (text.empty()) return;
    os << '\n' << text;
    if (text.back() != '\n') os << '\n';
}

std::string render_monologue_user(
    const std::string& name,
    const CharacterCore& core,
    const std::vector<MonologueLine>& lines,
    const std::string& perception) {
    std::ostringstream os;
    os << "You are " << name << ".\n";
    os << "\nWho you are:\n" << core.text;
    if (core.text.empty() || core.text.back() != '\n') os << '\n';
    for (const auto& line : lines)
        emit_piece(os, line.text);
    emit_piece(os, perception);
    return os.str();
}

constexpr char kPerceptionUserSentinel[] = "<<<RHAPSODE_PERCEPTION_USER>>>";

constexpr char kPerceptionJsonSchema[] =
    "{\"perception\":\"...\",\"facts\":[{\"fact\":\"...\",\"entities\":[]}]}";

std::string perception_system_instructions(const std::string& name,
                                           const std::string& who) {
    std::ostringstream os;
    os << "You are " << name << ", from the inside.\n";
    os << "Who you are:\n" << who;
    if (who.empty() || who.back() != '\n') os << '\n';
    os << '\n';
    os <<
        "The user text is the last three turns of narration around you.\n"
        "Write one first-person perception of what you took in. Skip the rest. "
        "Do not copy the scene. Empty is fine.\n"
        "Do not invent another person's private mind. No inventory.\n\n"
        "Answer with ONLY the JSON (no prose wrapper).\n\n"
        "JSON:\n" << kPerceptionJsonSchema << "\n";
    return os.str();
}

}  // namespace

std::string CharacterMemory::build_perception_prompt(
    const std::string& narration_window,
    const std::string& who) {
    ensure_bootstrap(who);
    return perception_system_instructions(character_name_, core_.text)
         + kPerceptionUserSentinel + "\n" + narration_window;
}

void CharacterMemory::apply_perception_json(int turn, const std::string& raw) {
    nlohmann::json parsed = try_parse_json(raw);
    if (parsed.is_null() || !parsed.is_object()) {
        const auto sliced = extract_json_object(raw);
        if (!sliced.empty())
            parsed = try_parse_json(sliced);
    }
    if (!parsed.is_object()) {
        log_warn("perception") << character_name_
              << " unparseable -- no-op apply\n";
        return;
    }
    if (parsed.contains("perception") && parsed["perception"].is_string()) {
        const std::string text =
            str::trim(sanitize_utf8(parsed["perception"].get<std::string>()));
        if (!text.empty())
            perception_ = text;
    }
    if (parsed.contains("facts") && parsed["facts"].is_array()) {
        std::vector<KnowFact> knows;
        for (const auto& value : parsed["facts"]) {
            if (!value.is_object())
                continue;
            KnowFact item;
            item.fact = str::trim(sanitize_utf8(value.value("fact", "")));
            if (item.fact.empty())
                continue;
            if (value.contains("entities") && value["entities"].is_array()) {
                for (const auto& entity : value["entities"])
                    if (entity.is_string())
                        item.entities.push_back(entity.get<std::string>());
            }
            knows.push_back(std::move(item));
        }
        apply_knows(beliefs_, turn, knows);
    }
    perception_turn_ = turn;
    auto& os = log_info("perception");
    os << character_name_ << " t=" << turn;
    if (perception_.empty())
        os << " (empty)";
    else
        os << " " << perception_;
    os << "\n" << std::flush;
}

void CharacterMemory::update_perception(int turn, const std::string& who,
                                        const std::string& narration_window,
                                        const LLMCallback& llm_callback) {
    if (!llm_callback) {
        log() << "  [perception:" << character_name_
              << "] skip: no LLM callback\n" << std::flush;
        return;
    }
    const std::string prompt = build_perception_prompt(narration_window, who);
    std::string raw;
    try {
        raw = llm_callback(prompt);
    } catch (const std::exception& ex) {
        log() << "  [perception:" << character_name_
              << "] update_perception failed: " << ex.what() << "\n";
        return;
    }
    apply_perception_json(turn, raw);
}

void CharacterMemory::ensure_bootstrap(const std::string& core_text_if_empty) {
    if (core_.text.empty() && !core_text_if_empty.empty())
        core_.text = sanitize_utf8(core_text_if_empty);
}

nlohmann::json CharacterMemory::render_mind_query(
    std::size_t max_belief_chars, std::size_t max_line_chars) const {
    nlohmann::json result;
    result["core"] = sanitize_utf8(
        truncate_utf8(core_.text, static_cast<int>(max_belief_chars / 2)));
    result["perception"] = sanitize_utf8(
        truncate_utf8(perception_, static_cast<int>(max_belief_chars / 2)));
    nlohmann::json recent = nlohmann::json::array();
    const int n = static_cast<int>(monologue_lines_.size());
    const int start = n > 3 ? n - 3 : 0;
    for (int i = start; i < n; ++i) {
        nlohmann::json line;
        line["turn"] = monologue_lines_[static_cast<std::size_t>(i)].turn;
        line["text"] = sanitize_utf8(truncate_utf8(
            monologue_lines_[static_cast<std::size_t>(i)].text,
            static_cast<int>(max_line_chars)));
        recent.push_back(std::move(line));
    }
    result["monologue"] = std::move(recent);
    result["beliefs"] = sanitize_utf8(
        truncate_utf8(render_thoughts({}), static_cast<int>(max_belief_chars)));
    return result;
}

std::string CharacterMemory::build_monologue_prompt(
    const std::string& description) {
    ensure_bootstrap(description);
    return monologue_system_instructions() + kMonologueUserSentinel + "\n"
         + render_monologue_user(character_name_, core_, monologue_lines_,
                                 perception_);
}

void CharacterMemory::apply_monologue_json(int turn, const std::string& raw) {
    nlohmann::json parsed = try_parse_json(raw);
    if (parsed.is_null() || !parsed.is_object()) {
        const auto sliced = extract_json_object(raw);
        if (!sliced.empty())
            parsed = try_parse_json(sliced);
    }
    if (!parsed.is_object()) {
        log() << "  [char_mem:" << character_name_
              << "] monologue unparseable -- no-op apply\n";
        return;
    }
    monologue_turn_ = turn;
    if (!parsed.contains("line") || parsed["line"].is_null())
        return;
    if (!parsed["line"].is_string())
        return;
    const std::string text =
        str::trim(sanitize_utf8(parsed["line"].get<std::string>()));
    if (text.empty())
        return;
    monologue_lines_.push_back({turn, text});
}

void CharacterMemory::update_monologues(int turn, const std::string& description,
                                        const LLMCallback& llm_callback) {
    const std::string prompt = build_monologue_prompt(description);
    if (!llm_callback) {
        log() << "  [char_mem:" << character_name_
              << "] monologue skip: no LLM callback\n" << std::flush;
        return;
    }
    std::string raw;
    try {
        raw = llm_callback(prompt);
    } catch (const std::exception& ex) {
        log() << "  [char_mem:" << character_name_
              << "] update_monologues failed: " << ex.what() << "\n";
        return;
    }
    apply_monologue_json(turn, raw);
}

}  // namespace rhapsode
