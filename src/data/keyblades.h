#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

struct KeybladeDef {
    std::string id;
    std::string name;
    std::string kind = "keyblade";       // "keyblade" | "sabre"
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
    uint32_t unlock_mission = 0;         // story mission that opens this weapon (0 = available once the master gate opens)
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
    int32_t index_for_id(const std::string& id) const;   // -1 if unknown

    // Story gate: Zero cannot wield a keyblade until after mission two's
    // flashback with Sora. Until then the twin red sabres carry him.
    const KeybladeDef* sabres() const { return find("twin_red_sabres"); }
    uint32_t gate_mission() const { return m_gate_mission; }
    bool keyblades_locked(uint32_t story_progress) const {
        return story_progress < m_gate_mission;
    }

private:
    std::vector<KeybladeDef> m_blades;
    uint32_t m_gate_mission = 2;   // master_gate.unlock_after_mission
};

} // namespace khz
