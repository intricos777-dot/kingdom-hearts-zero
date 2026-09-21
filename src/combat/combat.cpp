#include "combat/combat.h"
#include "ui/command_deck.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <iostream>

namespace khz {

namespace {
uint32_t roll(uint32_t lo, uint32_t hi) {
    if (hi <= lo) return lo;
    return lo + (uint32_t)(std::rand() % (hi - lo + 1));
}

std::string hp_bar(uint32_t cur, uint32_t max, uint32_t width) {
    std::string s;
    uint32_t filled = max ? (cur * width / max) : 0;
    if (filled > width) filled = width;
    for (uint32_t i = 0; i < width; ++i) s += (i < filled) ? "\xE2\x96\x88" : "\xE2\x96\x91";
    return s;
}
}

CombatEngine::CombatEngine(SaveSystem& saves, const KeybladeDB& blades,
                           const MaterialCatalog* mats)
    {
    bind(saves, blades, mats);
}

void CombatEngine::bind(SaveSystem& saves, const KeybladeDB& blades,
                        const MaterialCatalog* mats) {
    m_saves = &saves;
    m_blades = &blades;
    m_mats = mats;
    std::srand((unsigned)std::time(nullptr));
    rebuild_deck();
}

// ---- deck assembly: KH2-style unlocks by level, themed by the world ----

void CombatEngine::rebuild_deck() {
    m_deck.clear();
    const uint32_t lvl = m_saves->record().level;
    m_deck.push_back({"Slash", "slash", 0, 10 + lvl / 3, true});
    if (lvl >= 1) m_deck.push_back({"Fira", "fire", 8, 24 + lvl / 2, true});
    if (lvl >= 3) m_deck.push_back({"Blizzard", "blizzard", 8, 22 + lvl / 2, true});
    if (lvl >= 5) m_deck.push_back({"Thunder", "thunder", 16, 34 + lvl / 2, true});
    if (lvl >= 7) m_deck.push_back({"Dark Side", "dark", 14, 30 + lvl / 2, true});
    m_deck.push_back({"Cura", "cure", 12, 50 + lvl * 2, true});
    if (lvl >= 4) m_deck.push_back({"Focus", "focus", 6, 20 + lvl / 2, true});
    m_deck.push_back({"Guard", "guard", 0, 0, true});
}

// ---- player stats = save base + active keyblade (+ form bonus) ----

namespace {
// The weapon the save record points at — but bounded by the story gate:
// untitled steel (sabres) always applies; keyblades only answer once the
// flashback with Sora has opened the Inheritance (story_progress >= gate).
const KeybladeDef* active_blade(const SaveRecord& rec, const KeybladeDB& db) {
    const KeybladeDef* b = db.by_index(rec.active_keyblade);
    if (!b) b = db.sabres();
    if (b && b->kind != "keyblade") return b;              // sabres: always yours
    if (db.keyblades_locked(rec.story_progress)) return db.sabres(); // gate closed
    return b;
}

// Twilight form: stats from owned blades (up to `limit` of the best).
template <typename T>
uint32_t twilight_sum(const SaveRecord& rec, const KeybladeDB& db, uint32_t limit,
                      T KeybladeDef::*field) {
    if (db.keyblades_locked(rec.story_progress)) return 0;  // no keyblades yet
    std::vector<const KeybladeDef*> owned;
    for (uint32_t i = 0; i < 32; ++i)
        if (rec.owned_keyblades & (1u << i))
            if (const KeybladeDef* b = db.by_index(i))
                if (b->kind == "keyblade") owned.push_back(b);
    std::sort(owned.begin(), owned.end(),
              [&](const KeybladeDef* a, const KeybladeDef* b) {
                  return (a->*field) > (b->*field);
              });
    uint32_t sum = 0;
    for (size_t i = 0; i < std::min<size_t>(limit, owned.size()); ++i) {
        int32_t v = (int32_t)(owned[i]->*field);
        if (v > 0) sum += (uint32_t)v;
    }
    return sum;
}
}

bool CombatEngine::keyblades_unlocked() const {
    return !m_blades || !m_blades->keyblades_locked(m_saves->record().story_progress);
}

std::string CombatEngine::weapon_name() const {
    if (const KeybladeDef* b = active_blade(m_saves->record(), *m_blades))
        return b->name;
    return "(bare hands)";
}

uint32_t CombatEngine::player_str(uint32_t idx) {
    const SaveRecord& rec = m_saves->record();
    uint32_t base = rec.base_str;
    if (m_form == FORM_TWILIGHT) return base + twilight_sum(rec, *m_blades, 5, &KeybladeDef::str);
    if (const KeybladeDef* b = active_blade(rec, *m_blades)) base += b->str;
    if (m_form == FORM_SHADOW) base = base * 3 / 2;
    return base;
}

uint32_t CombatEngine::player_mag(uint32_t idx) {
    const SaveRecord& rec = m_saves->record();
    uint32_t base = rec.base_mag;
    if (m_form == FORM_TWILIGHT) return base + twilight_sum(rec, *m_blades, 5, &KeybladeDef::mag);
    if (const KeybladeDef* b = active_blade(rec, *m_blades)) base += b->mag;
    if (m_form == FORM_ULTIMA) base = base * 3 / 2;
    return base;
}

uint32_t CombatEngine::player_def(uint32_t idx) {
    const SaveRecord& rec = m_saves->record();
    uint32_t base = rec.base_def;
    if (m_form == FORM_TWILIGHT) return base + twilight_sum(rec, *m_blades, 5, &KeybladeDef::def);
    if (const KeybladeDef* b = active_blade(rec, *m_blades)) base += b->def;
    return base;
}

uint32_t CombatEngine::player_crt(uint32_t idx) {
    const SaveRecord& rec = m_saves->record();
    uint32_t base = rec.base_crt;
    if (m_form == FORM_TWILIGHT) return base + twilight_sum(rec, *m_blades, 5, &KeybladeDef::crt);
    if (const KeybladeDef* b = active_blade(rec, *m_blades)) base += b->crt;
    return base;
}

int32_t CombatEngine::player_spd(uint32_t idx) {
    const SaveRecord& rec = m_saves->record();
    int32_t base = (int32_t)rec.base_spd;
    if (m_form == FORM_TWILIGHT) return base + (int32_t)twilight_sum(rec, *m_blades, 5, &KeybladeDef::spd);
    if (const KeybladeDef* b = active_blade(rec, *m_blades)) base += b->spd;
    return base;
}

// Active leader stat lookups (Zero = save+blade+form; Echo = flat resonance).
uint32_t CombatEngine::active_str() { return m_leader_zero ? player_str(0) : m_echo_str; }
uint32_t CombatEngine::active_mag() { return m_leader_zero ? player_mag(0) : m_echo_mag; }
uint32_t CombatEngine::active_def() { return m_leader_zero ? player_def(0) : m_echo_def; }
uint32_t CombatEngine::active_crt() { return m_leader_zero ? player_crt(0) : m_echo_crt; }

// ---- damage resolution ----

namespace {
uint32_t element_multiplier(const std::string& atk, const std::string& def) {
    if (def.empty() || def == "none") return 100;
    if (atk == def) return 75;                          // same element: resisted
    if ((atk == "fire" && def == "blizzard") ||
        (atk == "blizzard" && def == "fire") ||
        (atk == "thunder" && def == "water") ||
        (atk == "water" && def == "fire") ||
        (atk == "light" && def == "dark") ||
        (atk == "dark" && def == "light")) return 150;  // opposing: weak
    return 100;
}
}

uint32_t CombatEngine::resolve_attack(const Command& cmd, const EnemyDef& e, uint32_t dex) {
    (void)dex;
    auto& rec = m_saves->record();
    (void)rec;
    bool physical = (cmd.element == "slash" || cmd.element == "guard" ||
                     cmd.element == "dark");
    uint32_t stat = physical ? active_str() : active_mag();
    int32_t dmg = (int32_t)(cmd.power + stat) - (int32_t)(e.def / (physical ? 2 : 1));
    if (cmd.element == "guard") return 0;
    if (dmg < 1) dmg = 1;
    int32_t mult = (int32_t)element_multiplier(cmd.element, e.element);
    dmg = (int32_t)((uint32_t)dmg * mult / 100);

    // crit
    uint32_t crit_chance = active_crt() + (cmd.element == "dark" ? 3 : 0);
    bool crit = (crit_chance > 0) && ((uint32_t)std::rand() % 100) < crit_chance * 5;
    if (crit) dmg = (int32_t)((uint32_t)dmg * 150 / 100);

    // FF7R-flavored stagger: weakness hits and crits press the bar; a
    // staggered target takes 1.5x while its counter hangs broken.
    if (m_view.staggered) {
        dmg = (int32_t)((uint32_t)dmg * 3 / 2);
    } else {
        uint32_t gain = mult >= 150 ? 30u : (crit ? 20u : 10u);
        m_view.stagger = std::min(m_view.stagger_max, m_view.stagger + (float)gain);
        if (m_view.stagger >= m_view.stagger_max) {
            m_view.staggered = true;
            m_view.stagger = 0.0f;
            m_view.stagger_left = 2.0f;   // seconds of broken counter
        }
    }

    return (uint32_t)std::max(0, dmg);
}

uint32_t CombatEngine::enemy_attack_damage(const EnemyAttack& atk) {
    int32_t dmg = (int32_t)atk.power - (int32_t)active_def() / 2;
    dmg = (int32_t)((uint32_t)dmg * element_multiplier(atk.element, "none") / 100);
    if (dmg < 1) dmg = 1;
    return (uint32_t)dmg;
}

// ---- session ----

void CombatEngine::sync_view_fighter() {
    auto& rec = m_saves->record();
    m_view.leader_is_zero = m_leader_zero;
    m_view.echo_in = m_echo_in;
    m_view.echo_hp = m_echo_hp;
    m_view.echo_max_hp = m_echo_max_hp;
    m_view.echo_mp = m_echo_mp;
    m_view.echo_max_mp = m_echo_max_mp;
    m_view.partner_deck = m_echo_deck;
    if (m_leader_zero) {
        m_view.hp = rec.hp; m_view.max_hp = rec.max_hp;
        m_view.mp = rec.mp; m_view.max_mp = rec.max_mp;
        m_view.deck = m_deck;
    } else {
        m_view.hp = m_echo_hp; m_view.max_hp = m_echo_max_hp;
        m_view.mp = m_echo_mp; m_view.max_mp = m_echo_max_mp;
        m_view.deck = m_echo_deck;
    }
}

void CombatEngine::begin(const EnemyDef& enemy) {
    m_enemy = &enemy;
    m_result = BattleResult{};
    m_guard_next = 0;
    m_dodge_next = false;
    m_form = 0;
    m_view = BattleView{};
    // A segment is charged at the whistle; two add up to a cast.
    m_view.atb = 1.0f;
    m_view.atb_max = 2.0f;
    m_view.stagger_max = (float)(enemy.stagger > 0 ? enemy.stagger : 100);
    m_view.phase = BattlePhase::FormSelect;
    m_view.enemy_name = enemy.name;
    m_view.enemy_kind = enemy.kind;
    m_view.enemy_id = enemy.id;
    m_view.enemy_hp = enemy.hp;
    m_view.enemy_max_hp = enemy.hp;
    m_view.music = enemy.music;

    // The Echo: a resonance of the drowned shore, rebuilt fresh per battle.
    // It is never persisted - it is the fight's second breath, not a save.
    const uint32_t lvl = m_saves->record().level;
    m_echo_max_hp = 60 + lvl * 3;
    m_echo_max_mp = 30 + lvl * 2;
    m_echo_hp = m_echo_max_hp;
    m_echo_mp = m_echo_max_mp;
    m_echo_str = 14 + lvl / 2;
    m_echo_mag = 12 + lvl / 2;
    m_echo_def = 10 + lvl / 3;
    m_echo_crt = 4;
    m_echo_deck.clear();
    m_echo_deck.push_back({"Slash", "slash", 0, 12 + lvl / 3, true});
    m_echo_deck.push_back({"Fira", "fire", 8, 22 + lvl / 2, true});
    m_echo_deck.push_back({"Blizzard", "blizzard", 8, 20 + lvl / 2, true});
    m_echo_deck.push_back({"Cura", "cure", 12, 40 + lvl * 2, true});
    m_leader_zero = true;
    m_echo_in = true;

    rebuild_deck();
    sync_view_fighter();
}

std::vector<std::string> CombatEngine::form_names() const {
    const auto& forms = m_saves->record().forms_unlocked;
    std::vector<std::string> names;
    names.push_back("Base Form");                       // always 0
    if (forms & FORM_SHADOW)  names.push_back("Shadow Overdrive");
    if (forms & FORM_ULTIMA)  names.push_back("Ultima Drive");
    if (forms & FORM_TWILIGHT) names.push_back("Twilight Form");
    return names;
}

void CombatEngine::select_form(size_t choice) {
    switch (choice) {
        case 1: m_form = FORM_SHADOW; break;
        case 2: m_form = FORM_ULTIMA; break;
        case 3: m_form = FORM_TWILIGHT; break;
        default: m_form = 0;
    }
    rebuild_deck();
    sync_view_fighter();
    m_view.phase = BattlePhase::PlayerTurn;
}

// One full round: player command (ATB-gated), then (if still alive) the
// enemy reply - unless the stagger interrupts it, the dodge slips it, or
// the guard blocks it. Narration lines are returned for the host to render.
#define NARR(fmt, ...)                                      \
    do {                                                    \
        char _b[256];                                       \
        std::snprintf(_b, sizeof(_b), fmt __VA_OPT__(,) __VA_ARGS__); \
        log.push_back(_b);                                  \
    } while (0)

std::vector<std::string> CombatEngine::act(size_t deck_index) {
    auto& rec = m_saves->record();
    std::vector<std::string> log;
    if (m_view.phase != BattlePhase::PlayerTurn || !m_enemy) {
        return log;
    }
    const EnemyDef& enemy = *m_enemy;
    const std::vector<Command>& deck = m_leader_zero ? m_deck : m_echo_deck;
    if (deck_index >= deck.size()) return log;
    const Command& cmd = deck[deck_index];

    // ---- FF7R-flavored ATB gate: commands cost gauge segments ----
    const float cost = atb_cost_for(cmd);
    if (m_view.atb < cost) {
        NARR("(the gauge flickers - not enough charge)");
        return log;
    }
    m_view.atb = std::max(0.0f, m_view.atb - cost);
    if (cmd.element == "slash")
        m_view.atb = std::min(m_view.atb_max, m_view.atb + 0.12f);  // swings charge the bar

    // ---- player command ----
    const char* who = m_leader_zero ? "Zero" : "the Echo";
    if (cmd.element == "cure") {
        uint32_t& hp = m_leader_zero ? rec.hp : m_echo_hp;
        uint32_t& maxhp = m_leader_zero ? rec.max_hp : m_echo_max_hp;
        uint32_t heal = std::min(maxhp - hp, cmd.power);
        hp += heal;
        NARR("%s casts Cura - restored %u HP (%u/%u)", who, heal, hp, maxhp);
        sync_view_fighter();
    } else if (cmd.element == "focus") {
        uint32_t gain = std::min(rec.max_mp - rec.mp, cmd.power);
        rec.mp += gain;
        m_view.mp = rec.mp;
        NARR("Zero focuses - recovered %u MP (%u/%u)", gain, rec.mp, rec.max_mp);
    } else if (cmd.element == "guard") {
        m_guard_next = active_def() * 2 + 8;
        NARR("%s raises the guard", who);
    } else {
        if (m_leader_zero ? rec.mp < cmd.cost : m_echo_mp < cmd.cost) {
            NARR("(not enough MP - the card flickers)");
            return log;
        }
        if (m_leader_zero) {
            rec.mp -= cmd.cost;
            m_view.mp = rec.mp;
        } else {
            m_echo_mp -= cmd.cost;
            m_view.mp = m_echo_mp;
        }
        uint32_t dmg = resolve_attack(cmd, enemy, 0);
        m_view.enemy_hp = (dmg >= m_view.enemy_hp) ? 0 : m_view.enemy_hp - dmg;
        NARR("%s: %s strikes for %u damage%s", who, cmd.name.c_str(), dmg,
             (m_view.staggered ? " - STAGGER!" : ""));
        if (m_view.enemy_hp == 0) {
            on_victory(log);
            return log;
        }
    }

    // ---- enemy reply: interrupted by stagger, slipped by dodge, blocked ----
    uint32_t dmg = 0;
    if (m_view.staggered) {
        NARR("%s reels - its counter is broken.", enemy.name.c_str());
    } else if (m_dodge_next) {
        m_dodge_next = false;
        NARR("you slip through the dark's reach.");
    } else if (m_guard_next > 0) {
        NARR("%s is blocked by the guard.", enemy.name.c_str());
        m_guard_next = 0;
    } else if (!enemy.attacks.empty() &&
               (m_view.enemy_kind == "shambler" || (std::rand() % 5) == 0)) {
        const EnemyAttack& atk = enemy.attacks[std::rand() % enemy.attacks.size()];
        if (atk.effect == "steal" && rec.memories_held > 0 && m_view.enemy_kind == "shambler") {
            --rec.memories_held;
            ++m_view.memories_stolen;
            NARR("%s steals a memory from you! (%u left)", enemy.name.c_str(),
                 rec.memories_held);
        } else {
            dmg = enemy_attack_damage(atk);
            if (atk.effect == "debuff") dmg = (dmg * 3) / 2;
            NARR("%s uses %s - %u damage.", enemy.name.c_str(), atk.name.c_str(), dmg);
        }
    } else {
        dmg = std::max<uint32_t>(1, enemy.str + 2 - active_def() / 2);
        NARR("%s strikes for %u damage.", enemy.name.c_str(), dmg);
    }

    // Damage lands on the active leader. The Echo unravels instead of dying;
    // Zero's death is the defeat.
    if (m_leader_zero) {
        rec.hp = (dmg >= rec.hp) ? 0 : rec.hp - dmg;
        m_view.hp = rec.hp;
        if (rec.hp == 0) {
            NARR("[The dark closes around Zero...]");
            m_view.phase = BattlePhase::Defeat;
            m_result = BattleResult{};
            m_result.victory = false;
            return log;
        }
    } else {
        m_echo_hp = (dmg >= m_echo_hp) ? 0 : m_echo_hp - dmg;
        if (m_echo_hp == 0) {
            NARR("[The Echo unravels - it can answer no more]");
            m_echo_in = false;
            m_leader_zero = true;   // the fight is handed back to Zero
        }
        sync_view_fighter();
    }
    m_view.phase = BattlePhase::PlayerTurn;
    return log;
}

void CombatEngine::tick_atb(float dt) {
    if (m_view.phase != BattlePhase::PlayerTurn) return;
    m_view.atb = std::min(m_view.atb_max, m_view.atb + dt);
    if (m_view.staggered) {
        m_view.stagger_left -= dt;
        if (m_view.stagger_left <= 0.0f) {
            m_view.staggered = false;
            m_view.stagger = 0.0f;
        }
    }
}

bool CombatEngine::atb_ready(size_t i) const {
    const std::vector<Command>& deck = m_leader_zero ? m_deck : m_echo_deck;
    if (i >= deck.size()) return false;
    return m_view.atb >= atb_cost_for(deck[i]);
}

void CombatEngine::dodge_up() {
    if (m_view.phase == BattlePhase::PlayerTurn) m_dodge_next = true;
}

std::vector<std::string> CombatEngine::swap_leader() {
    std::vector<std::string> log;
    if (m_view.phase != BattlePhase::PlayerTurn || !m_enemy) return log;
    if (m_view.atb < 1.0f) {
        NARR("(the gauge needs a full segment - the resonance demurs)");
        return log;
    }
    if (m_leader_zero && !m_echo_in) {
        NARR("(the Echo is unraveled - only Zero can stand)");
        return log;
    }
    m_view.atb = std::max(0.0f, m_view.atb - 1.0f);
    m_leader_zero = !m_leader_zero;
    if (m_leader_zero) NARR("The Echo recedes; Zero steps into the light.");
    else NARR("Zero falls back; the Echo answers in his place.");
    sync_view_fighter();
    return log;
}
#undef NARR

void CombatEngine::on_victory(std::vector<std::string>& log) {
    const EnemyDef& enemy = *m_enemy;
    bool boss = (m_view.enemy_kind == "shambler");
    m_view.phase = BattlePhase::Victory;
    m_result.victory = true;
    m_result.xp = enemy.exp * (boss ? 2 : 1);
    m_result.memories_stolen = m_view.memories_stolen;

    char b[256];
    std::snprintf(b, sizeof(b), "[%s falls] +%u XP", enemy.name.c_str(), m_result.xp);
    log.push_back(b);
    std::printf("  \x1b[38;5;220m%s\x1b[0m\n", b);

    // boss loot: the keyblade the boss guards, once
    if (boss && !enemy.loot_keyblade.empty()) {
        size_t idx = 0;
        for (const auto& k : m_blades->all()) {
            if (k.id == enemy.loot_keyblade) break;
            ++idx;
        }
        if (idx < m_blades->all().size()) {
            uint32_t bit = (1u << idx);
            if (!(m_saves->record().owned_keyblades & bit)) {
                m_saves->record().owned_keyblades |= bit;
                const KeybladeDef& k = m_blades->all()[idx];
                m_result.loot_keyblade = k.id;
                m_result.loot_desc = k.desc;
                std::snprintf(b, sizeof(b), "[KEYBLADE EARNED] %s - %s",
                              k.name.c_str(), k.desc.c_str());
                log.push_back(b);
                std::printf("  \x1b[38;5;220m\x1b[1m%s\x1b[0m\n", b);
            }
        }
    }

    // crafting motes: world- and arc-timed drops. Every defeated Heartless
    // and Shambler settles its account with the stall.
    auto& rec = m_saves->record();
    for (const auto& dr : enemy.drops) {
        if (dr.arc > rec.story_progress) continue;   // the story has not caught up
        if ((uint32_t)std::rand() % 1000 >= dr.chance_per_mille) continue;
        uint32_t qty = roll(dr.qty_min, dr.qty_max);
        if (qty == 0) continue;
        rec.materials[dr.material] += qty;
        m_result.loot.push_back(LootDrop{dr.material, qty});
        const char* matname = m_mats ? m_mats->name(dr.material).c_str() : "mote";
        std::snprintf(b, sizeof(b), "[mote] +%u %s", qty, matname);
        log.push_back(b);
        std::printf("  \x1b[38;5;34m%s\x1b[0m\n", b);
    }
    uint32_t munny = enemy.exp * (boss ? 4 : 2);
    if (munny > 0) {
        rec.munny += munny;
        m_result.munny = munny;
        std::snprintf(b, sizeof(b), "[munny] +%u the stall remembers your account", munny);
        log.push_back(b);
        std::printf("  \x1b[38;5;34m%s\x1b[0m\n", b);
    }
}

// ---- XP / leveling (KH2-style) ----

void CombatEngine::reward(const BattleResult& r) {
    auto& rec = m_saves->record();
    if (!r.victory) return;
    uint32_t before = rec.level;
    rec.xp += r.xp;
    while (rec.xp >= rec.xp_to_next) {
        rec.xp -= rec.xp_to_next;
        ++rec.level;
        rec.max_hp += 6;
        rec.max_mp += 3;
        rec.base_str += (rec.level % 2 == 0) ? 1 : 0;
        rec.base_mag += (rec.level % 2 == 0) ? 1 : 0;
        rec.base_def += (rec.level % 3 == 0) ? 1 : 0;
        rec.base_spd += (rec.level % 4 == 0) ? 1 : 0;
        rec.base_crt += (rec.level % 5 == 0) ? 1 : 0;
        rec.xp_to_next = 30 + rec.level * 12;
        rec.hp = rec.max_hp;
        rec.mp = rec.max_mp;
        std::printf("  \x1b[38;5;220m\x1b[1m[LEVEL UP] Zero is now level %u\x1b[0m\n", rec.level);
    }
    m_result.levels_gained = rec.level - before;
    rebuild_deck();
    sync_view_fighter();
}

// ---- terminal wrapper over the shared session ----

BattleResult CombatEngine::battle(const EnemyDef& enemy) {
    begin(enemy);

    if (m_view.phase == BattlePhase::FormSelect) {
        auto names = form_names();
        std::printf("\n  \x1b[1mChoose your form:\x1b[0m\n");
        for (size_t i = 0; i < names.size(); ++i)
            std::printf("   %zu) %s\n", i + 1, names[i].c_str());
        std::printf("  \x1b[2m[form]\x1b[0m ");
        std::string line;
        std::getline(std::cin, line);
        size_t choice = line.empty() ? 0 : (size_t)(line[0] - '1');
        if (choice >= names.size()) choice = 0;
        select_form(choice);
    }

    while (m_view.phase == BattlePhase::PlayerTurn) {
        // ATB + stagger strip
        std::printf("  \x1b[1mATB\x1b[0m %s %0.1f/%0.1f",
                    hp_bar((uint32_t)(m_view.atb / m_view.atb_max * 20.0f), 20, 20).c_str(),
                    m_view.atb, m_view.atb_max);
        if (m_view.staggered)
            std::printf("   \x1b[38;5;220m\x1b[1m[STAGGERING!]\x1b[0m\n");
        else
            std::printf("   \x1b[2m[stagger %0.0f/%0.0f]\x1b[0m\n",
                        m_view.stagger, m_view.stagger_max);

        // combatants
        std::printf("  \x1b[38;5;196m%s\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u\n",
                    m_view.enemy_name.c_str(),
                    hp_bar(m_view.enemy_hp, m_view.enemy_max_hp, 24).c_str(),
                    m_view.enemy_hp, m_view.enemy_max_hp);
        std::printf("  \x1b[1m%s\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u  MP \x1b[38;5;27m%s\x1b[0m %u/%u\n",
                    m_view.leader_is_zero ? "Zero" : "the Echo",
                    hp_bar(m_view.hp, m_view.max_hp, 24).c_str(), m_view.hp, m_view.max_hp,
                    hp_bar(m_view.mp, m_view.max_mp, 16).c_str(), m_view.mp, m_view.max_mp);
        if (m_view.echo_in)
            std::printf("  \x1b[2mEcho %s: HP %u/%u  MP %u/%u\x1b[0m\n",
                        m_view.leader_is_zero ? "(waiting)" : "(leading)",
                        m_view.echo_hp, m_view.echo_max_hp,
                        m_view.echo_mp, m_view.echo_max_mp);
        else
            std::printf("  \x1b[2mEcho: unraveled\x1b[0m\n");

        // deck with gauge costs
        std::printf("  \x1b[2m[deck]\x1b[0m ");
        for (size_t i = 0; i < m_view.deck.size(); ++i) {
            float c = m_view.atb_cost(i);
            const char* tag = c == 0.0f ? "free" : (c < 1.0f ? "0.5" : "1.0");
            std::printf("%zu) %s\x1b[2m(%uMP,%s)\x1b[0m  ", i + 1,
                        m_view.deck[i].name.c_str(), m_view.deck[i].cost, tag);
        }
        std::printf("\n  \x1b[2m[action]\x1b[0m [1-%zu cast] [a]ttack [g]uard [d]odge "
                    "[t]ick [s]wap [q]uit : ",
                    m_view.deck.size());
        std::string line;
        if (!std::getline(std::cin, line)) { m_result.escaped = true; return m_result; }
        char k = line.empty() ? '\0' : (char)line[0];

        if (k == 'q') { m_result.escaped = true; return m_result; }
        if (k == 'a') {
            for (auto& l : act(0)) std::printf("   %s\n", l.c_str());
        } else if (k == 'g') {
            size_t gi = 0;
            for (size_t i = 0; i < m_view.deck.size(); ++i)
                if (m_view.deck[i].element == "guard") { gi = i; break; }
            for (auto& l : act(gi)) std::printf("   %s\n", l.c_str());
        } else if (k == 'd') {
            dodge_up();
            std::printf("   you prepare to slip aside\n");
        } else if (k == 't') {
            tick_atb(0.4f);
            std::printf("   the dark waits; the gauge charges\n");
        } else if (k == 's') {
            for (auto& l : swap_leader()) std::printf("   %s\n", l.c_str());
        } else {
            int choice = std::atoi(line.c_str());
            if (choice < 1 || (size_t)choice > m_view.deck.size()) {
                std::printf("  \x1b[2m(the command is forgotten - the dark murmurs)\x1b[0m\n");
                continue;
            }
            for (auto& l : act((size_t)choice - 1)) std::printf("   %s\n", l.c_str());
        }
    }

    if (m_view.phase == BattlePhase::Victory) {
        std::printf("\n  \x1b[38;5;220m[%s falls.]\x1b[0m\n", m_view.enemy_name.c_str());
    } else if (m_view.phase == BattlePhase::Defeat) {
        std::printf("\n  \x1b[38;5;196m[The dark closes around Zero...]\x1b[0m\n");
    }
    return m_result;
}

} // namespace khz