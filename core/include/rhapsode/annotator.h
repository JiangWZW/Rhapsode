#pragma once

#include <functional>
#include <string>
#include <vector>

namespace rhapsode {

class World;

struct EntitySpan {
    int start;
    int end;
    std::string text;
    std::string category;
};

using NERCallback = std::function<std::string(const std::string& text)>;

class Annotator {
public:
    explicit Annotator(const World& world);
    void set_ner_callback(NERCallback cb);
    std::vector<EntitySpan> annotate(const std::string& text) const;

private:
    const World& world_;
    NERCallback ner_cb_;

    std::vector<EntitySpan> match_characters(const std::string& text) const;
    static std::vector<EntitySpan> merge(std::vector<EntitySpan> roster,
                                         std::vector<EntitySpan> ner);
};

}  // namespace rhapsode
