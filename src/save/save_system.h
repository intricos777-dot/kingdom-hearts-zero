#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

// Drive form bits, stored in SaveRecord::forms_unlocked. Shared by the
// combat engine and the moogle stall (which forges these as items).
constexpr uint32_t FORM_SHADOW   = (1u << 0);  // Shadow Overdrive
constexpr uint32_t FORM_ULTIMA   = (1u << 1);  // Ultima Drive
constexpr uint32_t FORM_TWILIGHT = (1u << 2);  // Twilight Form

// Persistent player state. Saves are checksummed; corruption self-heals.
struct SaveRecord {
    uint32_t version = 3;
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
    uint32_t owned_keyblades = 0x1;  // bit0 = twin_red_sabres
    uint32_t active_keyblade = 0;      // index of the equipped weapon
    // Drive forms: bit0 shadow overdrive, bit1 ultima drive, bit2 twilight form
    uint32_t forms_unlocked = 0;
    // Boss kills: bit per Shambler (defeated order == json order)
    uint32_t bosses_defeated = 0;
    // Command deck persistent state
    char active_deck[256];
    uint32_t active_deck_len = 0;
    uint32_t deck_level = 1; // unlocks slots as player progresses
    // Story progression: missions cleared. The master gate (see
    // data/combat/keyblades.json) opens at mission 2 — after the flashback
    // with Sora grants Zero the ability to wield keyblades.
    uint32_t story_progress = 0;      // 0=before m1, 1=after m1, 2=after m2 (keyblade gate open), ...
    // Stall economy: the dark pays in motes. materials[] index == material
    // catalog index (data/crafting/material_catalog.json). Drops are rolled
    // on victory, keyed to the enemy's world and the current story arc.
    uint32_t munny = 0;
    uint32_t materials[32] = {0};
};
;

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
