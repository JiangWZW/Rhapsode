add_library(rhapsode STATIC
    include/rhapsode/annotator.h
    include/rhapsode/character.h
    include/rhapsode/character_memory.h
    include/rhapsode/director.h
    include/rhapsode/history.h
    include/rhapsode/md5.h
    include/rhapsode/memory_system.h
    include/rhapsode/log_util.h
    include/rhapsode/narrator_prompt.h
    include/rhapsode/node.h
    include/rhapsode/scene.h
    include/rhapsode/json_util.h
    include/rhapsode/scene_loop.h
    include/rhapsode/scene_loop_support.h
    include/rhapsode/scene_message.h
    include/rhapsode/str_util.h
    include/rhapsode/text_downsampler.h
    include/rhapsode/validator.h
    include/rhapsode/weaver.h
    include/rhapsode/world_graph.h
    src/annotator.cpp
    src/scene_message.cpp
    src/history.cpp
    src/character.cpp
    src/character_memory.cpp
    src/narrator_prompt.cpp
    src/scene.cpp
    src/scene_loop.cpp
    src/scene_loop_support.cpp
    src/text_downsampler.cpp
    src/node.cpp
    src/director.cpp
    src/memory_system.cpp
    src/validator.cpp
    src/weaver.cpp
    src/world_graph.cpp
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

# Tests
add_executable(test_scene EXCLUDE_FROM_ALL tests/test_scene.cpp)
target_link_libraries(test_scene PRIVATE rhapsode Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(test_scene)
