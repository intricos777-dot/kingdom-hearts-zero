#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "data/enemies.h"
#include "data/keyblades.h"
#include "crafting/materials.h"
#include "save/save_system.h"
#include "ui/command_deck.h"

namespace khz {

enum class Form {
    Base    = 0,
    Shadow  = 1,   // Shadow Overdrive: heartless form
    Ultima  = 2,   // Ultima Drive
    Twilight = 4,  // blends Shadow + Ultima, 5 keyblades at once
};

// Form bits live in save_system.h (they are the save's forms_unlocked mask).

// Result of a single battle.
struct BattleResult {
    bool victory = false;
    bool escaped = false;
    uint32_t xp = 0;
    uint32_t levels_gained = 0;
    uint32_t memories_stolen = 0;   // how many memories the dark took from you
    uint32_t munny = 0;             // the dark settles its account
    std::vector<LootDrop> loot;     // crafting motes dropped this fight
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

// FF7R-flavored ATB: a deck command's gauge cost.
//   slash  -> free (basic swings build the bar)
//   guard  -> 0.5 segment (free block at a price)
//   spells -> 1.0 segment
inline float atb_cost_for(const Command& c) {
    if (c.element == "slash") return 0.0f;
    if (c.element == "guard") return 0.5f;
    return 1.0f;
}

// Snapshot of a session for HUD rendering.
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

    // FF7R-flavored ATB: commands cost gauge segments; the host feeds time.
    float atb = 0.0f, atb_max = 2.0f;
    float stagger = 0.0f, stagger_max = 100.0f;
    bool staggered = false;
    float stagger_left = 0.0f;    // seconds of stagger remaining

    // The active fighter: Zero, or the Echo (a resonance of the drowned
    // shore; rebuilt fresh every battle, never persisted).
    bool leader_is_zero = true;
    bool echo_in = true;
    uint32_t echo_hp = 0, echo_max_hp = 0;
    uint32_t echo_mp = 0, echo_max_mp = 0;
    std::vector<Command> partner_deck;   // the Echo's deck while it leads

    float atb_cost(size_t i) const { return i < deck.size() ? atb_cost_for(deck[i]) : 0.0f; }
};

// FF7R-flavored command combat over the deck engine.
// - Free movement time is the judge's silence: commands cost ATB segments
//   charged by the host (terminal 't'/attack push, shell frame time).
// - Weakness hits and crits press the stagger bar; at full, the dark is
//   staggered (1.5x damage in, counter interrupted, drains over time).
// - Keep the flow deterministic: the engine only ever reads dt it is fed.
// - Enemies (Heartless + Shambler bosses) attack back after each command.
// - Shambler memory-steal subtracts memories_held & un-realms command cards.
// - Victory grants XP (KH2-style leveling) and boss keyblade loot.
class CombatEngine {
public:
    CombatEngine() = default;
    CombatEngine(SaveSystem& saves, const KeybladeDB& blades,
                 const MaterialCatalog* mats = nullptr);

    // Bind save + keyblade DB (used when default-constructed by a host
    // that wires dependencies after construction).
    void bind(SaveSystem& saves, const KeybladeDB& blades,
              const MaterialCatalog* mats = nullptr);

    // Fights the given enemy until one side falls. Returns the result.
    BattleResult battle(const EnemyDef& enemy);

    // Session API (terminal or GUI host drives the loop).
    void begin(const EnemyDef& enemy);
    const BattleView& view() const { return m_view; }
    const BattleResult& result() const { return m_result; }

    // Host picks deck command index; advances one full round (player +
    // enemy) and returns the round's narration lines. Gated by ATB.
    std::vector<std::string> act(size_t deck_index);

    // FF7R-flavored ATB + stagger. The host feeds elapsed time; a command
    // is executable once its gauge cost is charged.
    void tick_atb(float dt);
    bool atb_ready(size_t i) const;
    void dodge_up();                        // free: next enemy reply misses
    std::vector<std::string> swap_leader(); // costs 1.0 atb; Zero <-> the Echo

    // Form select phase (shamblers only).
    std::vector<std::string> form_names() const;   // labels of form choices
    void select_form(size_t choice);               // 0 = base

    // Applies XP gains + level-ups (call after victory), prints growth.
    void reward(const BattleResult& r);

    // Try to unlock a drive form (returns true and prints flavor on success).
    bool unlock_form(uint32_t form_bit, const std::string& flavor);

    // The deck currently available to the player, shaped by level + form.
    const std::vector<Command>& deck() const { return m_view.deck; }

    // Story gate: until the flashback after mission two, Zero cannot wield
    // keyblades — the twin red sabres are the only weapon the engine allows.
    bool keyblades_unlocked() const;
    std::string weapon_name() const;   // active melee: sabres or equipped keyblade

protected:
    SaveSystem* m_saves = nullptr;
    const KeybladeDB* m_blades = nullptr;
    const MaterialCatalog* m_mats = nullptr;

    std::vector<Command> m_deck;
    uint32_t m_form = 0;   // active form bit

    BattleView m_view;
    BattleResult m_result;
    const EnemyDef* m_enemy = nullptr;   // set by begin()
    uint32_t m_guard_next = 0;
    bool m_dodge_next = false;
    bool m_enemy_turn_first = false;

    // Active leader: Zero, or the Echo (fresh per battle, never persisted).
    bool m_leader_zero = true;
    bool m_echo_in = true;
    std::vector<Command> m_echo_deck;
    uint32_t m_echo_hp = 0, m_echo_max_hp = 0;
    uint32_t m_echo_mp = 0, m_echo_max_mp = 0;
    uint32_t m_echo_str = 0, m_echo_mag = 0, m_echo_def = 0, m_echo_crt = 0;

    void on_victory(std::vector<std::string>& log);

    uint32_t player_str(uint32_t idx);
    uint32_t player_mag(uint32_t idx);
    uint32_t player_def(uint32_t idx);
    uint32_t player_crt(uint32_t idx);
    int32_t  player_spd(uint32_t idx);

    // Leader-aware stat lookups (Zero = save + blade + form; Echo = flat).
    uint32_t active_str();
    uint32_t active_mag();
    uint32_t active_def();
    uint32_t active_crt();

    uint32_t resolve_attack(const Command& cmd, const EnemyDef& e, uint32_t dex);
    uint32_t enemy_attack_damage(const EnemyAttack& atk);

    // Copies fighter state into the HUD view (hp/mp/deck + partner strip).
    void sync_view_fighter();

    void rebuild_deck();
};

} // namespace khz