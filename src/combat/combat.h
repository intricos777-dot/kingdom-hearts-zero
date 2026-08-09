#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "data/enemies.h"
#include "data/keyblades.h"
#include "save/save_system.h"
#include "ui/command_deck.h"

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

// Headless battle session state, driven by whichever UI (terminal or the
// visual shell) is attached. The engine applies rules; the host renders.
enum class BattlePhase {
    None,
    FormSelect,   // bosses only: pick a drive form
    PlayerTurn,   // awaiting a deck command
    Victory,
    Defeat,
    Escaped,
};

// Sanpshot of a session for HUD rendering.
struct BattleView {
    BattlePhase phase = BattlePhase::None;
    std::string enemy_name;
    std::string enemy_kind;   // "shambler" | "heartless"
    std::string enemy_id;     // manifest id (seal ledger key)
    uint32_t enemy_hp = 0;
    uint32_t enemy_max_hp = 0;
    uint32_t hp = 0, max_hp = 0;
    uint32_t mp = 0, max_mp = 0;
    uint32_t memories_stolen = 0;
    std::vector<Command> deck;
    uint32_t mp_cost(size_t i) const { return i < deck.size() ? deck[i].cost : 0; }
    std::string music;        // boss fight track clip id
};

// Turn-based menu combat over the command deck.
// - Player picks from the world-themed deck (CommandDeck).
// - Enemies (Heartless + Shambler bosses) attack back.
// - Shambler memory-steal subtracts memories_held & un-realms command cards.
// - Victory grants XP (KH2-style leveling) and boss keyblade loot.
class CombatEngine {
public:
    CombatEngine() = default;
    CombatEngine(SaveSystem& saves, const KeybladeDB& blades);

    // Bind save + keyblade DB (used when default-constructed by a host
    // that wires dependencies after construction).
    void bind(SaveSystem& saves, const KeybladeDB& blades);

    // Fights the given enemy until one side falls. Returns the result.
    BattleResult battle(const EnemyDef& enemy);

    // Session API (terminal or GUI host drives the loop).
    void begin(const EnemyDef& enemy);
    const BattleView& view() const { return m_view; }
    const BattleResult& result() const { return m_result; }

    // Host picks deck command index; advances one full round (player +
    // enemy) and returns the round's narration lines.
    std::vector<std::string> act(size_t deck_index);

    // Form select phase (shamblers only).
    std::vector<std::string> form_names() const;   // labels of form choices
    void select_form(size_t choice);               // 0 = base

    // Applies XP gains + level-ups (call after victory), prints growth.
    void reward(const BattleResult& r);

    // Try to unlock a drive form (returns true and prints flavor on success).
    bool unlock_form(uint32_t form_bit, const std::string& flavor);

    // The deck currently available to the player, shaped by level + form.
    const std::vector<Command>& deck() const { return m_view.deck; }

protected:
    SaveSystem* m_saves = nullptr;
    const KeybladeDB* m_blades = nullptr;

    std::vector<Command> m_deck;
    uint32_t m_form = 0;   // active form bit

    BattleView m_view;
    BattleResult m_result;
    const EnemyDef* m_enemy = nullptr;   // set by begin()
    uint32_t m_guard_next = 0;
    bool m_enemy_turn_first = false;

    void on_victory(std::vector<std::string>& log);

    uint32_t player_str(uint32_t idx);
    uint32_t player_mag(uint32_t idx);
    uint32_t player_def(uint32_t idx);
    uint32_t player_crt(uint32_t idx);
    int32_t  player_spd(uint32_t idx);

    uint32_t resolve_attack(const Command& cmd, const EnemyDef& e, uint32_t dex);
    uint32_t enemy_attack_damage(const EnemyAttack& atk);

    void rebuild_deck();
};

} // namespace khz