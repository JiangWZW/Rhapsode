pybind11_add_module(_core bind_rhapsode.cpp)
target_link_libraries(_core PRIVATE rhapsode)

add_custom_command(TARGET _core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:_core>
        "${CMAKE_SOURCE_DIR}/server/rhapsode/_core.pyd"
    COMMENT "Copying _core.pyd to server/rhapsode/"
)
