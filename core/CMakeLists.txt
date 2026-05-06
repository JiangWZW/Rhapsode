add_library(rhapsode STATIC
    include/rhapsode/character.h
    include/rhapsode/director.h
    include/rhapsode/history.h
    include/rhapsode/node.h
    include/rhapsode/node_pool.h
    include/rhapsode/scene.h
    include/rhapsode/scene_loop.h
    include/rhapsode/scene_message.h
    src/scene_message.cpp
    src/history.cpp
    src/character.cpp
    src/scene.cpp
    src/scene_loop.cpp
    src/node.cpp
    src/node_pool.cpp
    src/director.cpp
)

target_include_directories(rhapsode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rhapsode PUBLIC nlohmann_json::nlohmann_json)

# Tests
add_executable(test_scene EXCLUDE_FROM_ALL tests/test_scene.cpp)
target_link_libraries(test_scene PRIVATE rhapsode Catch2::Catch2WithMain)
add_executable(test_node_pool EXCLUDE_FROM_ALL tests/test_node_pool.cpp)
target_link_libraries(test_node_pool PRIVATE rhapsode Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(test_scene)
catch_discover_tests(test_node_pool)
