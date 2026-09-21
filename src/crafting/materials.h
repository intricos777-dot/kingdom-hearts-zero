#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

// A crafting material slot: index == SaveRecord::materials[] index.
struct MatDef {
    std::string id;
    std::string name;
    std::string rarity;   // common | uncommon | rare | epic | legendary
    std::string desc;
};

// One drop roll attached to an enemy. The material index is resolved at load
// time against the material catalog, so battles never parse JSON.
struct EnemyDrop {
    uint8_t material = 0;     // index into MaterialCatalog
    uint32_t qty_min = 1;
    uint32_t qty_max = 1;
    uint32_t chance_per_mille = 1000;
    uint32_t arc = 1;         // minimum story_progress at which this drop unlocks
};

// Item handed to the player after a victory (index + quantity).
struct LootDrop {
    uint8_t material = 0;
    uint32_t qty = 0;
};

// The Forge's material registry (32 fixed slots, index = save slot).
class MaterialCatalog {
public:
    bool load(const std::string& path);

    size_t size() const { return m_mats.size(); }

    int32_t index_by_id(const std::string& id) const;   // -1 if unknown
    const MatDef* by_index(uint32_t i) const;
    std::string name(uint32_t i) const { return i < m_mats.size() ? m_mats[i].name : "???"; }

private:
    std::vector<MatDef> m_mats;
};

} // namespace khz