#include "render/worlds.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <cstdio>

namespace khz {

bool WorldDB::load_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[Worlds] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Worlds] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    std::string game = j.value("game", "kh");
    for (const auto& w : j["worlds"]) {
        WorldDef d;
        d.id = w.value("id", "");
        d.name = w.value("name", d.id);
        d.game = w.value("game", game);
        d.timeline = w.value("timeline", "");
        d.why = w.value("why", "");
        d.story = w.value("story", "");
        d.shader = w.value("shader", "ps2_kh1");
        for (const auto& b : w["beats"]) d.beats.push_back(b.get<std::string>());
        m_worlds.push_back(std::move(d));
    }
    return true;
}

bool WorldDB::load(const std::string& kh1_path, const std::string& kh2_path) {
    bool a = load_file(kh1_path);
    bool b = load_file(kh2_path);
    return a && b;
}

void WorldDB::layout_circle(float radius) {
    size_t n = m_worlds.size();
    for (size_t i = 0; i < n; ++i) {
        float ang = (float)i / (float)n * 2.0f * 3.14159265f - 3.14159265f / 2.0f;
        m_worlds[i].x = std::cos(ang) * radius;
        m_worlds[i].z = std::sin(ang) * radius;
        // node color: kh1 worlds blue-violet, kh2 worlds red-amber
        if (m_worlds[i].game == "kh2") {
            m_worlds[i].r = 1.0f;
            m_worlds[i].g = 0.45f;
            m_worlds[i].b = 0.15f;
        } else {
            m_worlds[i].r = 0.25f;
            m_worlds[i].g = 0.75f;
            m_worlds[i].b = 1.0f;
        }
    }
}

} // namespace khz
