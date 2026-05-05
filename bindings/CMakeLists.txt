pybind11_add_module(_core bind_rhapsode.cpp)
target_link_libraries(_core PRIVATE rhapsode)
