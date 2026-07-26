#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "rhapsode/character.h"
#include "rhapsode/eval/session_report.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story.h"
#include "rhapsode/world.h"

using namespace rhapsode;
namespace fs = std::filesystem;

namespace {

fs::path temp_run_dir() {
    const auto path = fs::temp_directory_path() /
        ("rhapsode_eval_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(path);
    return path;
}

}  // namespace

TEST_CASE("SessionReport reliability parses turns and log markers",
          "[session_eval]") {
    const fs::path dir = temp_run_dir();
    {
        std::ofstream manifest(dir / "manifest.json");
        manifest << R"({
          "end_reason": "TurnTimeout",
          "max_turns": 3,
          "server_exit_code": 0,
          "timeouts": 1,
          "errors": 0
        })";
    }
    {
        std::ofstream turns(dir / "turns.jsonl");
        turns << R"({"turn":1,"input":"look","t_ms":1200.0,"status":"ok"})" << "\n";
        turns << R"({"turn":2,"input":"talk","t_ms":5000.0,"status":"timeout"})" << "\n";
    }
    {
        std::ofstream log(dir / "console.log");
        log << "normal line\nTurn failed with exception: boom\nrollback complete\n";
    }

    const SessionReport report = SessionReport::from_run_dir(dir.string());
    REQUIRE(report.end_reason == EndReason::TurnTimeout);
    REQUIRE(report.reliability.turns_completed == 2);
    REQUIRE(report.reliability.turns_requested == 3);
    REQUIRE(report.reliability.timeouts >= 1);
    REQUIRE(report.reliability.turn_ms.size() == 2);
    REQUIRE_FALSE(report.reliability.log_markers.empty());

    report.write(dir.string());
    REQUIRE(fs::exists(dir / "report.md"));
    REQUIRE(fs::exists(dir / "report.json"));
    fs::remove_all(dir);
}

TEST_CASE("SessionReport narrative detects empty beats and cast gaps",
          "[session_eval]") {
    const fs::path dir = temp_run_dir();
    const fs::path saves = dir / "saves";
    fs::create_directories(saves);

    SceneData scene;
    scene.scene_id = "root";
    scene.title = "Root";
    scene.system_prompt = "Narrate.";
    scene.history = {
        {Role::User, "I enter.", "", {}},
        {Role::Assistant, "Hi", "", {}},  // empty-ish beat
        {Role::User, "I speak to the barkeep.", "", {}},
        {Role::Assistant,
         "The tavern is quiet and the fire crackles while rain drums the roof "
         "and the long wooden bar gleams under oil lamps.",
         "", {}},
        {Role::Assistant,
         "The tavern is quiet and the fire crackles while rain drums the roof "
         "and the long wooden bar gleams under oil lamps again.",
         "", {}},
    };
    scene.turn_index = 2;

    World world;
    world.enter_character("root", Character("Hero", "player", true));
    world.enter_character("root", Character("Mira", "barkeep", false));
    world.enter_character("root", Character("Unseen Ghost", "never appears", false));

    Story story = Story::from_data(std::move(scene), std::move(world));
    story.save(saves.string());

    {
        std::ofstream manifest(dir / "manifest.json");
        manifest << R"({"end_reason":"MaxTurns","max_turns":2})";
    }
    {
        std::ofstream turns(dir / "turns.jsonl");
        turns << R"({"turn":1,"input":"enter","t_ms":100.0,"status":"ok"})" << "\n";
        turns << R"({"turn":2,"input":"speak","t_ms":110.0,"status":"ok"})" << "\n";
    }
    std::ofstream(dir / "console.log") << "ok\n";

    const SessionReport report = SessionReport::from_run_dir(dir.string());
    REQUIRE(report.narrative.empty_beats >= 1);
    REQUIRE(report.narrative.cast_gaps >= 1);
    REQUIRE(report.narrative.repetition_score > 0.0);
    bool found_gap = false;
    for (const auto& f : report.narrative.findings) {
        if (f.find("Unseen Ghost") != std::string::npos) found_gap = true;
    }
    REQUIRE(found_gap);

    report.write(dir.string());
    REQUIRE(fs::exists(dir / "story.txt"));
    {
        std::ifstream in(dir / "story.txt");
        std::string body((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
        REQUIRE(body.find("## Main —") != std::string::npos);
        REQUIRE(body.find("[Player]") != std::string::npos);
        REQUIRE(body.find("I enter.") != std::string::npos);
    }

    fs::remove_all(dir);
}
