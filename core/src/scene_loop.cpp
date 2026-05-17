#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <string_view>

#include <nlohmann/json.hpp>

namespace rhapsode {

namespace {

constexpr char kJsonMarker[] = "\n<<<RHAPSODE_JSON>>>\n";

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

bool extract_balanced_json(std::string_view tail, std::string& out) {
    auto start = tail.find('{');
    if (start == std::string_view::npos)
        return false;
    int  depth      = 0;
    bool in_string  = false;
    bool escaped    = false;

    for (size_t i = start; i < tail.size(); ++i) {
        char c = tail[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else {
            if (c == '"')
                in_string = true;
            else if (c == '{')
                ++depth;
            else if (c == '}') {
                --depth;
                if (depth == 0) {
                    out.assign(tail.data() + start, tail.data() + i + 1);
                    return true;
                }
            }
        }
    }
    return false;
}

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw) {
    nlohmann::json plan = nlohmann::json::object();

    const auto marker_pos = raw.find(kJsonMarker);
    if (marker_pos != std::string::npos) {
        auto prose_piece = trim(raw.substr(0, marker_pos));
        auto json_piece  =
            trim(raw.substr(marker_pos + static_cast<size_t>(std::strlen(kJsonMarker))));
        if (!json_piece.empty()) {
            try {
                plan = nlohmann::json::parse(json_piece);
            } catch (const std::exception&) {
                std::string salvage;
                if (extract_balanced_json(json_piece, salvage)) {
                    try {
                        plan = nlohmann::json::parse(salvage);
                    } catch (...) {
                        std::cerr << "  [merge] JSON parse failed after marker — using empty plan\n";
                    }
                }
            }
        }
        return {prose_piece.empty() ? trim(raw.substr(0, marker_pos)) : prose_piece,
                std::move(plan)};
    }

    const auto last_brace = raw.rfind('{');
    if (last_brace != std::string::npos) {
        const auto tail_sv = std::string_view(raw).substr(last_brace);
        std::string frag;
        if (extract_balanced_json(tail_sv, frag)) {
            try {
                plan = nlohmann::json::parse(frag);
                return {trim(raw.substr(0, last_brace)), std::move(plan)};
            } catch (...) {
                // prose-only fallback
            }
        }
    }

    return {trim(std::move(raw)), nlohmann::json::object()};
}

std::vector<std::pair<std::string, std::string>> extract_speech_cues(const nlohmann::json& plan) {
    std::vector<std::pair<std::string, std::string>> cues;
    auto it = plan.find("speech_turns");
    if (it == plan.end() || !it->is_array())
        return cues;

    for (const auto& el : *it) {
        if (!el.is_object())
            continue;
        auto name = el.value("character", "");
        auto cue  = el.value("cue", "");
        if (name.empty() || cue.empty())
            continue;
        cues.emplace_back(name, cue);
    }
    return cues;
}

}  // namespace

void SceneLoop::load_scene(Scene& scene) {
    scene_ = &scene;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::submit_input(const std::string& text) {
    if (state_ != LoopState::WaitingForInput)
        throw std::runtime_error("Cannot submit input: loop is not waiting for input");

    state_ = LoopState::ProcessingInput;

    SceneMessage user_msg;
    user_msg.role    = Role::User;
    user_msg.content = text;
    scene_->history.append(std::move(user_msg));

    advance();
}

LoopState SceneLoop::state() const { return state_; }

void SceneLoop::set_prompt_callback(PromptCallback cb) { prompt_cb_ = std::move(cb); }

void SceneLoop::set_llm_callback(LLMCallback cb) { llm_cb_ = std::move(cb); }

void SceneLoop::set_turn_complete_callback(TurnCompleteCallback cb) {
    turn_complete_cb_ = std::move(cb);
}

void SceneLoop::set_character_synth_callback(CharacterSynthCallback cb) {
    char_synth_cb_ = std::move(cb);
}

void SceneLoop::set_director(Director* director) { director_ = director; }

const DirectorOutput& SceneLoop::last_director_output() const { return last_director_out_; }

std::vector<SceneMessage> SceneLoop::take_last_turn_outputs() {
    auto out             = std::move(last_turn_outputs_);
    last_turn_outputs_ = {};
    return out;
}

std::string SceneLoop::build_scene_context() const {
    std::string ctx = scene_->title;

    auto recent = scene_->history.snapshot(6);
    for (const auto& msg : recent) {
        ctx += "\n";
        ctx += (msg.role == Role::User ? "user: " : "assistant: ");
        ctx += msg.content;
    }

    return ctx;
}

void SceneLoop::advance() {
    if (!prompt_cb_)
        throw std::runtime_error("No prompt callback registered");
    if (!llm_cb_)
        throw std::runtime_error("No LLM callback registered");

    std::cerr << "\n====== Turn " << scene_->turn_index << " ======\n";
    last_turn_outputs_.clear();

    state_ = LoopState::BuildingPrompt;

    const int graph_turn_idx = scene_->turn_index;

    std::cerr << "[1/4] Building merged prompt...\n" << std::flush;

    std::string focus_json = "{}";
    if (director_)
        focus_json = director_->focus_payload_json(graph_turn_idx, build_scene_context());

    size_t win = resuming_ ? resume_window_size_ : window_size_;
    auto history_snap = scene_->history.snapshot(win);
    resuming_           = false;

    std::string prompt =
        prompt_cb_(history_snap, *scene_, last_director_out_, focus_json);

    ++scene_->turn_index;

    {
        std::string sep(60, '-');
        std::cerr << "\n  " << sep << "\n  --- Merged prompt ---\n";
        for (size_t start = 0; start < prompt.size();) {
            auto end = prompt.find('\n', start);
            if (end == std::string::npos) end = prompt.size();
            std::cerr << "  | " << prompt.substr(start, end - start) << "\n";
            start = end + 1;
        }
        std::cerr << "  " << sep << "\n" << std::flush;
    }

    state_ = LoopState::RunningLLM;

    std::cerr << "[2/4] Calling merged narrative LLM...\n" << std::flush;
    std::string merged_raw = llm_cb_(prompt);
    std::cerr << "  response length: " << merged_raw.size() << " chars\n";

    auto [prose_chunk, turn_plan] = split_merged_response(std::move(merged_raw));

    state_ = LoopState::AppendingResult;

    auto speech_cues = extract_speech_cues(turn_plan);

    std::cerr << "[3/4] Applying graph...\n" << std::flush;
    last_director_out_ = {};
    if (director_)
        last_director_out_ = director_->apply_planned_turn(graph_turn_idx, turn_plan);

    SceneMessage narration;
    narration.role                   = Role::Assistant;
    narration.content                = std::move(prose_chunk);
    narration.metadata               = nlohmann::json::object();
    narration.metadata["scene_kind"] = "narrator";

    const std::string narration_snapshot = narration.content;

    scene_->history.append(std::move(narration));
    last_turn_outputs_.push_back(scene_->history.messages().back());

    if (turn_complete_cb_)
        turn_complete_cb_(scene_->history.messages().back());

    std::cerr << "[4/4] Character synthesis (" << speech_cues.size()
              << " cue(s))...\n";

    if (!speech_cues.empty() && static_cast<bool>(char_synth_cb_)) {
        auto spoken_lines = char_synth_cb_(speech_cues, narration_snapshot);

        if (spoken_lines.size() > speech_cues.size())
            spoken_lines.resize(speech_cues.size());
        while (spoken_lines.size() < speech_cues.size())
            spoken_lines.emplace_back("...");

        for (size_t i = 0; i < speech_cues.size(); ++i) {
            SceneMessage line;
            line.role                   = Role::Assistant;
            line.content                = std::move(spoken_lines[i]);
            line.metadata               = nlohmann::json::object();
            line.metadata["scene_kind"] = "character";
            line.metadata["speaker"]    = speech_cues[i].first;

            scene_->history.append(std::move(line));
            last_turn_outputs_.push_back(scene_->history.messages().back());

            if (turn_complete_cb_)
                turn_complete_cb_(scene_->history.messages().back());
        }
    } else if (!speech_cues.empty()) {
        std::cerr << "  (skipped — character synth unset)\n";
    }

    std::cerr << "====== Turn " << (scene_->turn_index - 1) << " done ======\n"
              << std::flush;

    state_ = LoopState::WaitingForInput;
}

void SceneLoop::set_history_window(size_t normal, size_t resume) {
    window_size_        = normal;
    resume_window_size_ = resume;
}

}  // namespace rhapsode
