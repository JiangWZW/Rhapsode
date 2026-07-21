add_library(rhapsode STATIC
    include/rhapsode/annotator.h
    include/rhapsode/character.h
    include/rhapsode/character_memory.h
    include/rhapsode/director.h
    include/rhapsode/history.h
    include/rhapsode/llm_callback.h
    include/rhapsode/memory_system.h
    include/rhapsode/log_util.h
    include/rhapsode/narrator_prompt.h
    include/rhapsode/node.h
    include/rhapsode/scene_data.h
    include/rhapsode/json_util.h
    include/rhapsode/scene_loop.h
    include/rhapsode/scene_message.h
    include/rhapsode/str_util.h
    include/rhapsode/text_downsampler.h
    include/rhapsode/weaver.h
    include/rhapsode/story.h
    include/rhapsode/world.h
    include/rhapsode/world_graph.h
    src/annotator.cpp
    src/scene_message.cpp
    src/history.cpp
    src/character.cpp
    src/character_memory.cpp
    src/character_memory_reflection.cpp
    src/narrator_prompt.cpp
    src/story.cpp
    src/story_advance.cpp
    src/story_serialization.cpp
    src/scene_loop.cpp
    src/scene_loop_background.cpp
    src/scene_loop_narrator.cpp
    src/text_downsampler.cpp
    src/node.cpp
    src/director.cpp
    src/memory_system.cpp
    src/weaver.cpp
    src/weaver_work_queue.cpp
    src/world.cpp
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

if(MSVC)
    target_compile_options(rhapsode PRIVATE /W4)
else()
    target_compile_options(rhapsode PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests
add_executable(test_scene EXCLUDE_FROM_ALL
    tests/test_scene.cpp
    tests/test_substrate_golden.cpp
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
