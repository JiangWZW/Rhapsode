#pragma once

#include <cstdlib>
#include <iostream>

namespace rhapsode {

inline bool verbose_logging_enabled() {
    return std::getenv("RHAPSODE_VERBOSE_LOG") != nullptr;
}

inline std::ostream& log() {
    return std::cerr;
}

}  // namespace rhapsode
