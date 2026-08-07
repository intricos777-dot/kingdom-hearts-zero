#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

struct EnemyAttack {
    std::string name;
    std::string element;
    uint32_t power = 0;
    std::string effect;  // "steal" | "debuff" | "aoe" | "copy" | "finisher"
};

struct EnemyDef {
    std::string id;
    std::string name;
    std::string kind;    // "heartless" | "shambler"
    std::string world;   // shamblers only: the world they guard
    uint32_t act = 1;
    uint32_t hp = 10;
    uint32_t str = 1;
    uint32_t mag = 1;
    uint32_t def = 1;
    uint32_t spd = 1;
    uint32_t crt = 1;
    uint32_t exp = 10;
    std::string element;
    std::string memory_steal;
    std::vector<EnemyAttack> attacks;
    std::string loot_keyblade;
    std::string loot_desc;
    std::string music;   // boss fight track clip id
    std::string desc;
};

// Acts structure pulled from the same manifest.
struct ActDef {
    uint32_t act = 1;
    std::string title;
    std::vector<std::string> worlds;
    std::string summary;
};

// Enemy/act registry loaded from data/combat/enemies.json.
class EnemyDB {
public:
    bool load(const std::string& path);

    const std::vector<EnemyDef>& heartless() const { return m_heartless; }
    const std::vector<EnemyDef>& shamblers() const { return m_shamblers; }
    const std::vector<ActDef>& acts() const { return m_acts; }

    const EnemyDef* shambler_for_world(const std::string& world) const;
    const EnemyDef* find_shambler(const std::string& id) const;

private:
    std::vector<EnemyDef> m_heartless;
    std::vector<EnemyDef> m_shamblers;
    std::vector<ActDef> m_acts;
};

} // namespace khz
