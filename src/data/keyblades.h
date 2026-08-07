#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

struct KeybladeDef {
    std::string id;
    std::string name;
    std::string wielder;
    std::string source;
    std::string element;
    uint32_t str = 0;
    uint32_t mag = 0;
    uint32_t def = 0;
    int32_t spd = 0;
    uint32_t crt = 0;
    uint32_t ap = 0;
    uint32_t unlock_level = 1;
    std::string unlock_desc;
    std::vector<std::string> abilities;
    std::string desc;
};

// Registry loaded from data/combat/keyblades.json. Order is stable: the
// bitmask in SaveRecord::owned_keyblades indexes this vector directly.
class KeybladeDB {
public:
    bool load(const std::string& path);
    const std::vector<KeybladeDef>& all() const { return m_blades; }
    const KeybladeDef* find(const std::string& id) const;
    const KeybladeDef* by_index(size_t i) const {
        return (i < m_blades.size()) ? &m_blades[i] : nullptr;
    }

private:
    std::vector<KeybladeDef> m_blades;
};

} // namespace khz
