#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

// Persistent player state. Saves are checksummed; corruption self-heals.
struct SaveRecord {
    uint32_t version = 2;
    char world[32] = "traverse_town";
    uint32_t act = 1;
    uint32_t hp = 100;
    uint32_t max_hp = 100;
    uint32_t mp = 60;
    uint32_t max_mp = 100;
    uint32_t keyblade_tier = 0;   // 0 Dusk, 1 Twilight, 2 Doorwarden
    uint32_t keyholes_closed = 0;
    uint32_t memories_held = 0;
    char journal[4096] = {0};
    // KH2-style leveling
    uint32_t level = 1;
    uint32_t xp = 0;
    uint32_t xp_to_next = 30;
    uint32_t base_str = 5;
    uint32_t base_mag = 5;
    uint32_t base_def = 3;
    uint32_t base_spd = 3;
    uint32_t base_crt = 1;
    // Keyblade ownership: bitmask index into data/combat/keyblades.json
    uint32_t owned_keyblades = 0x1;  // bit0 = twilight_keyblade
    uint32_t active_keyblade = 0;      // index of the equipped blade
    // Drive forms: bit0 shadow overdrive, bit1 ultima drive, bit2 twilight form
    uint32_t forms_unlocked = 0;
    // Boss kills: bit per Shambler (defeated order == json order)
    uint32_t bosses_defeated = 0;
};

class SaveSystem {
public:
    SaveSystem() = default;

    bool initialize();
    bool save(const std::string& path);
    bool load(const std::string& path);

    SaveRecord& record() { return m_record; }
    const SaveRecord& record() const { return m_record; }
    bool corrupted() const { return m_corrupted; }

    static std::string default_path();

private:
    SaveRecord m_record;
    SaveRecord m_shadow;
    bool m_corrupted = false;
    uint32_t compute_checksum(const SaveRecord& rec) const;
    bool self_heal();
};

} // namespace khz
