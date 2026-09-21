#include "data/keyblades.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace khz {

bool KeybladeDB::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[Keyblades] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Keyblades] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    m_blades.clear();
    // Story gate: keyblades unlock after mission two's flashback with Sora.
    if (j.contains("master_gate"))
        m_gate_mission = j["master_gate"].value("unlock_after_mission", 2u);
    for (const auto& k : j.value("keyblades", nlohmann::json::array())) {
        KeybladeDef d;
        d.id = k.value("id", "");
        d.name = k.value("name", d.id);
        d.kind = k.value("kind", "keyblade");
        d.wielder = k.value("wielder", "");
        d.source = k.value("source", "");
        d.element = k.value("element", "none");
        d.str = k.value("str", 0);
        d.mag = k.value("mag", 0);
        d.def = k.value("def", 0);
        d.spd = k.value("spd", 0);
        d.crt = k.value("crt", 0);
        d.ap = k.value("ap", 0);
        d.unlock_level = k.value("unlock_level", 1);
        d.unlock_mission = k.value("unlock_mission", 0);
        d.unlock_desc = k.value("unlock_desc", "");
        d.desc = k.value("desc", "");
        for (const auto& a : k.value("abilities", nlohmann::json::array()))
            d.abilities.push_back(a.get<std::string>());
        m_blades.push_back(std::move(d));
    }
    std::printf("\x1b[2m[Keyblades] %zu blades remembered\x1b[0m\n", m_blades.size());
    return !m_blades.empty();
}

const KeybladeDef* KeybladeDB::find(const std::string& id) const {
    for (const auto& b : m_blades)
        if (b.id == id) return &b;
    return nullptr;
}

int32_t KeybladeDB::index_for_id(const std::string& id) const {
    for (size_t i = 0; i < m_blades.size(); ++i)
        if (m_blades[i].id == id) return (int32_t)i;
    return -1;
}

} // namespace khz
