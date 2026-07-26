pybind11_add_module(_core
    bind_rhapsode.cpp
    bind_story.cpp
    bind_graph.cpp
    bind_runtime.cpp
    bind_memory.cpp
    bind_eval.cpp
)
target_link_libraries(_core PRIVATE rhapsode)

if(MSVC)
    target_compile_options(_core PRIVATE /W4)
else()
    target_compile_options(_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET _core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:_core>
        "${CMAKE_SOURCE_DIR}/server/rhapsode/_core.pyd"
    COMMENT "Copying _core.pyd to server/rhapsode/"
)
