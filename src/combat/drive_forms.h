#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace khz {

// A fully-resolved drive form: name, visual theme, mechanical effect on stats,
// and special attacks granted.
struct DriveFormDef {
    std::string id;
    std::string name;        // "Shadow Overdrive", "Ultima Drive", "Twilight Form"
    std::string sigil;       // glyph shown in the command deck header
    uint32_t bit;            // 1, 2, 4 — matches FORM_SHADOW / FORM_ULTIMA / FORM_TWILIGHT
    std::string color_ansi;  // ansi 256 accent
    std::string flavor;      // short lore blurb
    // Stat multipliers (percent, applied to base save stats + keyblade bonuses).
    uint32_t str_mul = 100;
    uint32_t mag_mul = 100;
    uint32_t def_mul = 100;
    int32_t  spd_add = 0;
    uint32_t crt_add = 0;
    uint32_t mp_drain_per_turn = 0;
    // Special attacks unlocked while in this form (Command element tags).
    std::vector<std::string> specials;  // e.g. "shadowstep", "ultima_burst", "twilight_cleave"
    uint32_t unlock_story_progress = 0; // mission count at which this form unlocks
};

// Drive form registry. Loads from data/combat/drive_forms.json.
class DriveFormDb {
public:
    bool load(const std::string& path);

    const std::vector<DriveFormDef>& forms() const { return m_forms; }
    const DriveFormDef* by_bit(uint32_t bit) const;
    const DriveFormDef* by_id(const std::string& id) const;

    // Forms available at a given story progress.
    std::vector<const DriveFormDef*> available(uint32_t story_progress) const;

private:
    std::vector<DriveFormDef> m_forms;
};

} // namespace khz
