#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "data/enemies.h"
#include "data/keyblades.h"
#include "save/save_system.h"

namespace khz {

enum class Form {
    Base    = 0,
    Shadow  = 1,   // Shadow Overdrive: heartless form
    Ultima  = 2,   // Ultima Drive
    Twilight = 4,  // blends Shadow + Ultima, 5 keyblades at once
};

// Machine numbers used by the save bitmask.
constexpr uint32_t FORM_SHADOW  = (1u << 0);
constexpr uint32_t FORM_ULTIMA  = (1u << 1);
constexpr uint32_t FORM_TWILIGHT = (1u << 2);

// Result of a single battle.
struct BattleResult {
    bool victory = false;
    bool escaped = false;
    uint32_t xp = 0;
    uint32_t levels_gained = 0;
    uint32_t memories_stolen = 0;   // how many memories the dark took from you
    std::string loot_keyblade;       // keyblade id earned, or ""
    std::string loot_desc;
};

// Turn-based menu combat over the command deck.
// - Player picks from the world-themed deck (CommandDeck).
// - Enemies (Heartless + Shambler bosses) attack back.
// - Shambler memory-steal subtracts memories_held & un-realms command cards.
// - Victory grants XP (KH2-style leveling) and boss keyblade loot.
class CombatEngine {
public:
    CombatEngine(SaveSystem& saves, const KeybladeDB& blades);

    // Fights the given enemy until one side falls. Returns the result.
    BattleResult battle(const EnemyDef& enemy);

    // Applies XP gains + level-ups (call after victory), prints growth.
    void reward(const BattleResult& r);

    // Try to unlock a drive form (returns true and prints flavor on success).
    bool unlock_form(uint32_t form_bit, const std::string& flavor);

    // The deck currently available to the player, shaped by level + form.
    const std::vector<Command>& deck() const { return m_deck; }

protected:
    SaveSystem& m_saves;
    const KeybladeDB& m_blades;

    std::vector<Command> m_deck;
    uint32_t m_form = 0;   // active form bit

    uint32_t player_str(uint32_t idx);
    uint32_t player_mag(uint32_t idx);
    uint32_t player_def(uint32_t idx);
    uint32_t player_crt(uint32_t idx);
    int32_t  player_spd(uint32_t idx);

    uint32_t resolve_attack(const Command& cmd, const EnemyDef& e, uint32_t dex);
    uint32_t enemy_attack_damage(const EnemyAttack& atk, uint32_t& memories);

    void rebuild_deck();
};

} // namespace khz