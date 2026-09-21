#include "world/areas.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace khz {

bool AreaDb::load_file(const std::string& path, const std::string& game_tag) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[Areas] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Areas] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    std::string game = j.value("game", game_tag);
    for (const auto& w : j["worlds"]) {
        std::string world_id = w.value("id", "");
        std::string world_name = w.value("name", world_id);
        if (!w.contains("areas")) continue;
        for (const auto& a : w["areas"]) {
            AreaDef d;
            d.id = a.value("id", "");
            d.name = a.value("name", d.id);
            d.world = a.value("world", world_id);
            d.desc = a.value("desc", "");
            if (a.contains("map")) {
                for (const auto& r : a["map"])
                    d.rows.push_back(r.get<std::string>());
            }
            if (a.contains("npcs")) {
                for (const auto& n : a["npcs"])
                    d.npcs.push_back(n.get<std::string>());
            }
            d.music = a.value("music", "");
            m_areas.push_back(std::move(d));
        }
    }
    return true;
}

bool AreaDb::load_kh1(const std::string& path) {
    return load_file(path, "kh1");
}

bool AreaDb::load_kh2(const std::string& path) {
    return load_file(path, "kh2");
}

bool AreaDb::load(const std::string& kh1_path, const std::string& kh2_path) {
    bool a = load_kh1(kh1_path);
    bool b = load_kh2(kh2_path);
    return a || b;
}

std::vector<const AreaDef*> AreaDb::in_world(const std::string& world) const {
    std::vector<const AreaDef*> out;
    for (const auto& a : m_areas)
        if (a.world == world) out.push_back(&a);
    return out;
}

const AreaDef* AreaDb::find(const std::string& id) const {
    for (const auto& a : m_areas)
        if (a.id == id) return &a;
    return nullptr;
}

size_t AreaDb::area_count(const std::string& world) const {
    size_t c = 0;
    for (const auto& a : m_areas)
        if (a.world == world) ++c;
    return c;
}

} // namespace khz
