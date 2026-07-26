#include "rhapsode/eval/session_report.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
#include "rhapsode/world.h"

namespace rhapsode {
namespace {

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> tokens(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            cur.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

double jaccard(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.empty() || b.empty()) return 0.0;
    std::unordered_set<std::string> sa(a.begin(), a.end());
    std::unordered_set<std::string> sb(b.begin(), b.end());
    size_t inter = 0;
    for (const auto& t : sa) {
        if (sb.count(t)) ++inter;
    }
    const size_t uni = sa.size() + sb.size() - inter;
    return uni == 0 ? 0.0 : static_cast<double>(inter) / static_cast<double>(uni);
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

ReliabilityMetrics compute_reliability(const fs::path& dir) {
    ReliabilityMetrics m;
    const fs::path manifest_path = dir / "manifest.json";
    if (fs::exists(manifest_path)) {
        json manifest = json::parse(read_file(manifest_path), nullptr, false);
        if (manifest.is_object()) {
            m.turns_requested = manifest.value("max_turns", 0);
            m.server_exit_code = manifest.value("server_exit_code", 0);
            m.timeouts = manifest.value("timeouts", 0);
            m.errors = manifest.value("errors", 0);
        }
    }

    const fs::path turns_path = dir / "turns.jsonl";
    if (fs::exists(turns_path)) {
        std::ifstream in(turns_path);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            json row = json::parse(line, nullptr, false);
            if (!row.is_object()) continue;
            ++m.turns_completed;
            if (row.contains("t_ms") && row["t_ms"].is_number())
                m.turn_ms.push_back(row["t_ms"].get<double>());
            const std::string status = row.value("status", "");
            if (status == "timeout") ++m.timeouts;
            if (status == "error") ++m.errors;
        }
    }

    const std::string log = to_lower(read_file(dir / "console.log"));
    static const char* markers[] = {
        "exception", "rollback", "re-entry", "reentry", "fatal", "terminate",
    };
    for (const char* marker : markers) {
        if (log.find(marker) != std::string::npos)
            m.log_markers.emplace_back(marker);
    }
    return m;
}

NarrativeMetrics compute_narrative(const fs::path& saves_dir) {
    NarrativeMetrics m;
    if (!fs::exists(saves_dir / "world.json")) {
        m.findings.push_back("No saves/world.json — narrative metrics skipped");
        return m;
    }

    Story story;
    try {
        if (!story.has_save(saves_dir.string())) {
            m.findings.push_back("Save present but Story::has_save returned false");
            return m;
        }
        story.load_save(saves_dir.string());
    } catch (const std::exception& ex) {
        m.findings.push_back(std::string("Failed to load save: ") + ex.what());
        return m;
    }

    std::vector<std::string> assistant_texts;
    for (const auto& scene_id : story.scene_ids()) {
        const SceneData* scene = story.get_scene(scene_id);
        if (!scene) continue;
        for (const auto& msg : scene->history) {
            if (msg.role == Role::Assistant) assistant_texts.push_back(msg.content);
        }
        for (const auto& msg : scene->dialogue) {
            if (msg.role == Role::Assistant) assistant_texts.push_back(msg.content);
        }
    }

    std::vector<double> lengths;
    lengths.reserve(assistant_texts.size());
    for (const auto& text : assistant_texts) {
        const auto toks = tokens(text);
        lengths.push_back(static_cast<double>(toks.size()));
        if (toks.size() < 5) ++m.empty_beats;
    }

    double rep_sum = 0.0;
    int rep_n = 0;
    for (size_t i = 1; i < assistant_texts.size(); ++i) {
        rep_sum += jaccard(tokens(assistant_texts[i - 1]), tokens(assistant_texts[i]));
        ++rep_n;
    }
    m.repetition_score = rep_n == 0 ? 0.0 : rep_sum / rep_n;

    if (lengths.size() >= 4) {
        const size_t mid = lengths.size() / 2;
        double early = 0.0, late = 0.0;
        for (size_t i = 0; i < mid; ++i) early += lengths[i];
        for (size_t i = mid; i < lengths.size(); ++i) late += lengths[i];
        early /= static_cast<double>(mid);
        late /= static_cast<double>(lengths.size() - mid);
        m.length_collapse = early <= 1e-6 ? 0.0 : std::max(0.0, (early - late) / early);
    }

    const World& world = story.world();
    std::string corpus;
    for (const auto& t : assistant_texts) {
        corpus += to_lower(t);
        corpus.push_back('\n');
    }
    for (const auto& character : world.characters()) {
        if (character.is_player) continue;
        const std::string name = to_lower(character.name);
        if (name.size() < 2) continue;
        if (corpus.find(name) == std::string::npos) {
            ++m.cast_gaps;
            m.findings.push_back("Cast gap: \"" + character.name + "\" never mentioned in assistant text");
        }
    }

    if (m.empty_beats > 0)
        m.findings.push_back("Empty/near-empty assistant beats: " + std::to_string(m.empty_beats));
    if (m.repetition_score >= 0.45)
        m.findings.push_back("High consecutive repetition score: " +
                             std::to_string(m.repetition_score));
    if (m.length_collapse >= 0.4)
        m.findings.push_back("Narration length collapse: " +
                             std::to_string(m.length_collapse));
    if (m.findings.empty())
        m.findings.push_back("No narrative heuristic failures detected");
    return m;
}

std::string maybe_critique(const NarrativeMetrics& narrative,
                           const ReliabilityMetrics& reliability,
                           EndReason end_reason,
                           const LLMCallback& critique_llm) {
    if (!critique_llm) return {};
    std::ostringstream prompt;
    prompt << "You are reviewing a Rhapsode playtest session.\n"
           << "End reason: " << end_reason_name(end_reason) << "\n"
           << "Turns completed: " << reliability.turns_completed << "/"
           << reliability.turns_requested << "\n"
           << "Timeouts: " << reliability.timeouts
           << " Errors: " << reliability.errors << "\n"
           << "Narrative findings:\n";
    for (const auto& f : narrative.findings) prompt << "- " << f << "\n";
    prompt << "\nWrite a short critique covering: strengths, failures, stuckness. "
           << "Be concrete and concise.";
    try {
        return critique_llm(prompt.str());
    } catch (...) {
        return {};
    }
}

}  // namespace

std::string end_reason_name(EndReason reason) {
    switch (reason) {
        case EndReason::MaxTurns: return "MaxTurns";
        case EndReason::TurnTimeout: return "TurnTimeout";
        case EndReason::ServerExit: return "ServerExit";
        case EndReason::WsError: return "WsError";
        case EndReason::TurnError: return "TurnError";
    }
    return "MaxTurns";
}

EndReason end_reason_from_name(const std::string& name) {
    if (name == "TurnTimeout") return EndReason::TurnTimeout;
    if (name == "ServerExit") return EndReason::ServerExit;
    if (name == "WsError") return EndReason::WsError;
    if (name == "TurnError") return EndReason::TurnError;
    return EndReason::MaxTurns;
}

SessionReport SessionReport::from_run_dir(const std::string& dir,
                                          LLMCallback critique_llm) {
    SessionReport report;
    const fs::path root(dir);
    const fs::path manifest_path = root / "manifest.json";
    if (fs::exists(manifest_path)) {
        json manifest = json::parse(read_file(manifest_path), nullptr, false);
        if (manifest.is_object())
            report.end_reason =
                end_reason_from_name(manifest.value("end_reason", "MaxTurns"));
    }
    report.reliability = compute_reliability(root);
    report.narrative = compute_narrative(root / "saves");
    report.critique =
        maybe_critique(report.narrative, report.reliability, report.end_reason,
                       critique_llm);
    return report;
}

void SessionReport::write(const std::string& dir) const {
    const fs::path root(dir);
    fs::create_directories(root);

    json j;
    j["end_reason"] = end_reason_name(end_reason);
    j["reliability"] = {
        {"turns_completed", reliability.turns_completed},
        {"turns_requested", reliability.turns_requested},
        {"turn_ms", reliability.turn_ms},
        {"timeouts", reliability.timeouts},
        {"errors", reliability.errors},
        {"server_exit_code", reliability.server_exit_code},
        {"log_markers", reliability.log_markers},
    };
    j["narrative"] = {
        {"empty_beats", narrative.empty_beats},
        {"repetition_score", narrative.repetition_score},
        {"length_collapse", narrative.length_collapse},
        {"cast_gaps", narrative.cast_gaps},
        {"findings", narrative.findings},
    };
    j["critique"] = critique;

    {
        std::ofstream out(root / "report.json");
        out << j.dump(2);
    }

    std::ostringstream md;
    md << "# Session eval report\n\n";
    md << "- **End reason:** " << end_reason_name(end_reason) << "\n";
    md << "- **Turns:** " << reliability.turns_completed << " / "
       << reliability.turns_requested << "\n";
    md << "- **Timeouts:** " << reliability.timeouts
       << "  **Errors:** " << reliability.errors << "\n";
    md << "- **Server exit code:** " << reliability.server_exit_code << "\n";
    if (!reliability.turn_ms.empty()) {
        std::vector<double> sorted = reliability.turn_ms;
        std::sort(sorted.begin(), sorted.end());
        const double p50 = sorted[sorted.size() / 2];
        const double p95 = sorted[static_cast<size_t>(sorted.size() * 0.95)];
        md << "- **Turn latency ms:** p50=" << p50 << " p95=" << p95 << "\n";
    }
    if (!reliability.log_markers.empty()) {
        md << "- **Log markers:** ";
        for (size_t i = 0; i < reliability.log_markers.size(); ++i) {
            if (i) md << ", ";
            md << reliability.log_markers[i];
        }
        md << "\n";
    }
    md << "\n## Narrative\n\n";
    md << "- empty_beats=" << narrative.empty_beats
       << " repetition=" << narrative.repetition_score
       << " length_collapse=" << narrative.length_collapse
       << " cast_gaps=" << narrative.cast_gaps << "\n\n";
    md << "### Findings\n\n";
    for (const auto& f : narrative.findings) md << "- " << f << "\n";
    if (!critique.empty()) {
        md << "\n## Critique\n\n" << critique << "\n";
    }
    std::ofstream out(root / "report.md");
    out << md.str();

    const fs::path saves = root / "saves";
    if (fs::exists(saves / "world.json")) {
        try {
            Story story;
            if (story.has_save(saves.string())) {
                story.load_save(saves.string());
                std::ofstream story_out(root / "story.txt");
                story_out << story.render_transcript();
            }
        } catch (...) {
            // Transcript is best-effort; report already written.
        }
    }
}

}  // namespace rhapsode
