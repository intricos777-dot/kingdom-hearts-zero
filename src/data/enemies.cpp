#include "data/enemies.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace khz {

namespace {
EnemyDef parse_enemy(const nlohmann::json& e, const std::string& kind,
                     const MaterialCatalog& mats) {
    EnemyDef d;
    d.id = e.value("id", "");
    d.name = e.value("name", d.id);
    d.kind = kind;
    d.world = e.value("world", "");
    d.act = e.value("act", 1);
    d.hp = e.value("hp", 10);
    d.str = e.value("str", 1);
    d.mag = e.value("mag", 1);
    d.def = e.value("def", 1);
    d.spd = e.value("spd", 1);
    d.crt = e.value("crt", 1);
    d.exp = e.value("exp", 10);
    d.stagger = e.value("stagger", 100);
    d.element = e.value("element", "none");
    d.memory_steal = e.value("memory_steal", "");
    d.loot_keyblade = e.value("loot_keyblade", "");
    d.loot_desc = e.value("loot_desc", "");
    d.music = e.value("music", "");
    d.desc = e.value("desc", "");
    for (const auto& a : e.value("attacks", nlohmann::json::array())) {
        EnemyAttack at;
        at.name = a.value("name", "");
        at.element = a.value("element", "none");
        at.power = a.value("power", 0);
        at.effect = a.value("effect", "");
        d.attacks.push_back(std::move(at));
    }
    for (const auto& w : e.value("worlds", nlohmann::json::array()))
        d.worlds.push_back(w.get<std::string>());
    for (const auto& dr : e.value("drops", nlohmann::json::array())) {
        int32_t mi = mats.index_by_id(dr.value("material", ""));
        if (mi < 0 || mi >= 32) {
            std::fprintf(stderr, "[Enemies] %s: unknown drop material '%s'\n",
                         d.id.c_str(), dr.value("material", "").c_str());
            continue;
        }
        EnemyDrop drop;
        drop.material = (uint8_t)mi;
        drop.qty_min = dr.value("qty_min", 1);
        drop.qty_max = dr.value("qty_max", drop.qty_min);
        drop.chance_per_mille = dr.value("chance_per_mille", 1000);
        drop.arc = dr.value("arc", 1);
        d.drops.push_back(drop);
    }
    return d;
}
}

bool EnemyDB::load(const std::string& path, const MaterialCatalog& mats) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[Enemies] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Enemies] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    m_heartless.clear();
    m_shamblers.clear();
    m_acts.clear();
    for (const auto& e : j.value("heartless", nlohmann::json::array()))
        m_heartless.push_back(parse_enemy(e, "heartless", mats));
    for (const auto& e : j.value("shamblers", nlohmann::json::array()))
        m_shamblers.push_back(parse_enemy(e, "shambler", mats));
    for (const auto& a : j.value("acts", nlohmann::json::array())) {
        ActDef d;
        d.act = a.value("act", 1);
        d.title = a.value("title", "");
        for (const auto& w : a.value("worlds", nlohmann::json::array()))
            d.worlds.push_back(w.get<std::string>());
        d.summary = a.value("summary", "");
        m_acts.push_back(std::move(d));
    }
    std::printf("\x1b[2m[Enemies] %zu heartless, %zu shamblers, %zu acts\x1b[0m\n",
                m_heartless.size(), m_shamblers.size(), m_acts.size());
    return !m_shamblers.empty();
}

const EnemyDef* EnemyDB::shambler_for_world(const std::string& world) const {
    for (const auto& s : m_shamblers)
        if (s.world == world) return &s;
    return nullptr;
}

const EnemyDef* EnemyDB::find_shambler(const std::string& id) const {
    for (const auto& s : m_shamblers)
        if (s.id == id) return &s;
    return nullptr;
}

} // namespace khz