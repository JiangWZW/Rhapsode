#pragma once

#include <functional>
#include <string>

namespace rhapsode {

/// Generic LLM callback: prompt in, completion out.
/// Used by Director, Weaver, Validator, TextDownsampler, CharacterMemory, etc.
using LLMCallback = std::function<std::string(const std::string&)>;

}  // namespace rhapsode
