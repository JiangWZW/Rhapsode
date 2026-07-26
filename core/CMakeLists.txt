add_library(rhapsode STATIC
    include/rhapsode/annotator.h
    include/rhapsode/character.h
    include/rhapsode/character_memory.h
    include/rhapsode/director.h
    include/rhapsode/llm_callback.h
    include/rhapsode/memory_system.h
    include/rhapsode/log_util.h
    include/rhapsode/narrator_prompt.h
    include/rhapsode/node.h
    include/rhapsode/read_tools.h
    include/rhapsode/scene_data.h
    include/rhapsode/scene_history.h
    include/rhapsode/scenario_bootstrap.h
    include/rhapsode/json_util.h
    include/rhapsode/turn_executor.h
    include/rhapsode/scene_message.h
    include/rhapsode/str_util.h
    include/rhapsode/text_downsampling.h
    include/rhapsode/weaver.h
    include/rhapsode/story.h
    include/rhapsode/storyline_policy.h
    include/rhapsode/world.h
    include/rhapsode/world_analysis.h
    include/rhapsode/world_graph.h
    include/rhapsode/eval/session_eval.h
    include/rhapsode/eval/session_report.h
    src/annotator.cpp
    src/eval/session_eval.cpp
    src/eval/session_report.cpp
    src/scene_message.cpp
    src/scenario_bootstrap.cpp
    src/scene_history.cpp
    src/character.cpp
    src/character_memory.cpp
    src/character_memory_reflection.cpp
    src/narrator_prompt.cpp
    src/read_tools.cpp
    src/story.cpp
    src/story_advance.cpp
    src/story_serialization.cpp
    src/storyline_policy.cpp
    src/turn_executor.cpp
    src/turn_executor_post_turn.cpp
    src/turn_executor_narrator.cpp
    src/text_downsampling.cpp
    src/node.cpp
    src/director.cpp
    src/memory_system.cpp
    src/weaver.cpp
    src/weaver_work_queue.cpp
    src/world.cpp
    src/world_analysis.cpp
    src/world_graph.cpp
    src/world_graph_dot.cpp
    src/world_graph_serialization.cpp
)

set(RHAPSODE_LOCAL_BOOST_DIR "${CMAKE_SOURCE_DIR}/third_party/boost")
if(NOT EXISTS "${RHAPSODE_LOCAL_BOOST_DIR}/boost/graph/adjacency_list.hpp")
    message(FATAL_ERROR
        "Repo-local Boost not found at ${RHAPSODE_LOCAL_BOOST_DIR}. "
        "Run third_party/bootstrap_boost.bat (or build.bat which calls it).")
endif()

target_include_directories(rhapsode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${RHAPSODE_LOCAL_BOOST_DIR}
)

target_link_libraries(rhapsode PUBLIC nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(rhapsode PUBLIC ws2_32 mswsock)
    target_compile_definitions(rhapsode PUBLIC _WIN32_WINNT=0x0601)
endif()

if(MSVC)
    target_compile_options(rhapsode PRIVATE /W4)
else()
    target_compile_options(rhapsode PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests
add_executable(test_scene EXCLUDE_FROM_ALL
    tests/test_class_internals.cpp
    tests/test_scene.cpp
    tests/test_substrate_golden.cpp
    tests/test_session_report.cpp
)
target_link_libraries(test_scene PRIVATE rhapsode Catch2::Catch2WithMain)

if(MSVC)
    target_compile_options(test_scene PRIVATE /W4)
else()
    target_compile_options(test_scene PRIVATE -Wall -Wextra -Wpedantic)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(test_scene)
