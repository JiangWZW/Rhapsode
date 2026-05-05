# C++ data model

All core data types live in `core/include/rhapsode/`. JSON serialization uses [nlohmann/json](https://github.com/nlohmann/json) with ADL `to_json`/`from_json` free functions.

## SceneMessage

A single message in the conversation history.

```cpp
// include/rhapsode/scene_message.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace rhapsode {

enum class Role { System, User, Assistant };

struct SceneMessage {
    Role role;
    std::string content;
    std::string timestamp;  // ISO 8601, set on append

    // Optional extensibility without changing the struct layout
    nlohmann::json metadata = nlohmann::json::object();
};

NLOHMANN_JSON_SERIALIZE_ENUM(Role, {
    {Role::System,    "system"},
    {Role::User,      "user"},
    {Role::Assistant,  "assistant"},
})

void to_json(nlohmann::json& j, const SceneMessage& m);
void from_json(const nlohmann::json& j, SceneMessage& m);

} // namespace rhapsode
```

### JSON representation

```json
{
  "role": "user",
  "content": "I approach the bar and order an ale.",
  "timestamp": "2026-05-05T14:30:00Z",
  "metadata": {}
}
```

## History

Ordered collection of `SceneMessage`s. Owns the message storage for a scene.

```cpp
// include/rhapsode/history.h
#pragma once
#include <vector>
#include <optional>
#include "rhapsode/scene_message.h"

namespace rhapsode {

class History {
public:
    void append(SceneMessage msg);

    // Return the last n messages (or all if n > size).
    // Used by the prompt builder to get a context window.
    std::vector<SceneMessage> snapshot(std::optional<size_t> n = std::nullopt) const;

    size_t size() const;
    void clear();

    const std::vector<SceneMessage>& messages() const;

private:
    std::vector<SceneMessage> messages_;
};

void to_json(nlohmann::json& j, const History& h);
void from_json(const nlohmann::json& j, History& h);

} // namespace rhapsode
```

### Notes

- `snapshot(n)` returns a copy — the prompt builder in Python receives an immutable view.
- No max-length enforcement in MVP. The prompt builder is responsible for truncation.
- `timestamp` is set by `append()` using `std::chrono` (UTC ISO 8601).

## Character

Metadata about a participant in the scene.

```cpp
// include/rhapsode/character.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace rhapsode {

struct Character {
    std::string name;
    std::string description;
    bool is_player = false;
};

void to_json(nlohmann::json& j, const Character& c);
void from_json(const nlohmann::json& j, Character& c);

} // namespace rhapsode
```

### JSON representation

```json
{
  "name": "Barkeep",
  "description": "A gruff dwarf who runs the tavern",
  "is_player": false
}
```

## Scene

Top-level coordinator. Holds everything needed for a session.

```cpp
// include/rhapsode/scene.h
#pragma once
#include <string>
#include <vector>
#include "rhapsode/character.h"
#include "rhapsode/history.h"

namespace rhapsode {

class Scene {
public:
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;
    History history;

    // Load from / save to a JSON file on disk
    static Scene load_json(const std::string& path);
    void save_json(const std::string& path) const;

    // Serialize the full scene state (for WebSocket push or save)
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);
};

} // namespace rhapsode
```

### Full scene JSON

```json
{
  "title": "The Dusty Flagon",
  "system_prompt": "You are the narrator of a fantasy RPG...",
  "characters": [
    { "name": "Player", "description": "A wandering adventurer", "is_player": true },
    { "name": "Barkeep", "description": "A gruff dwarf who runs the tavern", "is_player": false }
  ],
  "history": [
    { "role": "assistant", "content": "You push open the heavy oak door...", "timestamp": "2026-05-05T14:30:00Z", "metadata": {} }
  ]
}
```

## Serialization contract

- All `to_json`/`from_json` functions are free functions in namespace `rhapsode`, following the nlohmann/json ADL pattern.
- Unknown JSON keys are ignored on deserialization (forward compatibility).
- `metadata` on `SceneMessage` is a pass-through `json` object — Python can attach arbitrary data without C++ needing to know the schema.
- File I/O (`load_json`/`save_json`) uses `std::ifstream`/`std::ofstream` with UTF-8 encoding.

## pybind11 exposure

All types above are exposed via pybind11 in `bindings/bind_rhapsode.cpp`:

| C++ type | Python name | Notes |
|----------|-------------|-------|
| `Role` | `rhapsode._core.Role` | Enum with `.System`, `.User`, `.Assistant` |
| `SceneMessage` | `rhapsode._core.SceneMessage` | Properties: `role`, `content`, `timestamp`, `metadata` |
| `History` | `rhapsode._core.History` | Methods: `append()`, `snapshot()`, `size()`, `clear()` |
| `Character` | `rhapsode._core.Character` | Properties: `name`, `description`, `is_player` |
| `Scene` | `rhapsode._core.Scene` | Properties + `load_json()`, `save_json()`, `to_json()` |
