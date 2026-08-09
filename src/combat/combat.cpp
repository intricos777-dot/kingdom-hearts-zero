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

CombatEngine::CombatEngine(SaveSystem& saves, const KeybladeDB& blades)
    {
    bind(saves, blades);
}

void CombatEngine::bind(SaveSystem& saves, const KeybladeDB& blades) {
    m_saves = &saves;
    m_blades = &blades;
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
const KeybladeDef* active_blade(const SaveRecord& rec, const KeybladeDB& db) {
    return db.by_index(rec.active_keyblade);
}

// Twilight form: stats from owned blades (up to `limit` of the best).
template <typename T>
uint32_t twilight_sum(const SaveRecord& rec, const KeybladeDB& db, uint32_t limit,
                      T KeybladeDef::*field) {
    std::vector<const KeybladeDef*> owned;
    for (uint32_t i = 0; i < 32; ++i)
        if (rec.owned_keyblades & (1u << i))
            if (const KeybladeDef* b = db.by_index(i)) owned.push_back(b);
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
    auto& rec = m_saves->record();
    bool physical = (cmd.element == "slash" || cmd.element == "guard" ||
                     cmd.element == "dark");
    uint32_t stat = physical ? player_str(dex) : player_mag(dex);
    int32_t dmg = (int32_t)(cmd.power + stat) - (int32_t)(e.def / (physical ? 2 : 1));
    if (cmd.element == "guard") return 0;
    if (dmg < 1) dmg = 1;
    dmg = (int32_t)((uint32_t)dmg * element_multiplier(cmd.element, e.element) / 100);

    // crit
    uint32_t crit_chance = player_crt(dex) + (cmd.element == "dark" ? 3 : 0);
    bool crit = (crit_chance > 0) && ((uint32_t)std::rand() % 100) < crit_chance * 5;
    if (crit) dmg = (int32_t)((uint32_t)dmg * 150 / 100);

    return (uint32_t)std::max(0, dmg);
}

uint32_t CombatEngine::enemy_attack_damage(const EnemyAttack& atk) {
    int32_t dmg = (int32_t)atk.power - (int32_t)player_def(0) / 2;
    dmg = (int32_t)((uint32_t)dmg * element_multiplier(atk.element, "none") / 100);
    if (dmg < 1) dmg = 1;
    return (uint32_t)dmg;
}

// ---- session ----

void CombatEngine::begin(const EnemyDef& enemy) {
    m_enemy = &enemy;
    m_result = BattleResult{};
    m_guard_next = 0;
    m_form = 0;
    m_view = BattleView{};
    m_view.phase = BattlePhase::FormSelect;
    m_view.enemy_name = enemy.name;
    m_view.enemy_kind = enemy.kind;
    m_view.enemy_id = enemy.id;
    m_view.enemy_hp = enemy.hp;
    m_view.enemy_max_hp = enemy.hp;
    m_view.hp = m_saves->record().hp;
    m_view.max_hp = m_saves->record().max_hp;
    m_view.mp = m_saves->record().mp;
    m_view.max_mp = m_saves->record().max_mp;
    m_view.music = enemy.music;
    rebuild_deck();
    m_view.deck = m_deck;
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
    m_view.deck = m_deck;
    m_view.phase = BattlePhase::PlayerTurn;
}

// One full round: player command, then (if still alive) the enemy reply.
// Narration lines are returned for the host to render in its own style.
#define NARR(fmt, ...)                                      \
    do {                                                    \
        char _b[256];                                       \
        std::snprintf(_b, sizeof(_b), fmt __VA_OPT__(,) __VA_ARGS__); \
        log.push_back(_b);                                  \
    } while (0)

std::vector<std::string> CombatEngine::act(size_t deck_index) {
    auto& rec = m_saves->record();
    std::vector<std::string> log;
    if (m_view.phase != BattlePhase::PlayerTurn || deck_index >= m_deck.size() ||
        !m_enemy) {
        return log;
    }
    const EnemyDef& enemy = *m_enemy;
    const Command& cmd = m_deck[deck_index];

    // ---- player command ----
    if (cmd.element == "cure") {
        uint32_t old = rec.hp;
        uint32_t heal = std::min(rec.max_hp - rec.hp, cmd.power);
        rec.hp += heal;
        m_view.hp = rec.hp;
        NARR("Zero casts Cura - restored %u HP (%u/%u)", heal, rec.hp, rec.max_hp);
    } else if (cmd.element == "focus") {
        uint32_t gain = std::min(rec.max_mp - rec.mp, cmd.power);
        rec.mp += gain;
        m_view.mp = rec.mp;
        NARR("Zero focuses - recovered %u MP (%u/%u)", gain, rec.mp, rec.max_mp);
    } else if (cmd.element == "guard") {
        m_guard_next = player_def(0) * 2 + 8;
        NARR("Zero raises the guard");
    } else {
        if (rec.mp < cmd.cost) {
            NARR("(not enough MP - the card flickers)");
            return log;
        }
        rec.mp -= cmd.cost;
        m_view.mp = rec.mp;
        uint32_t dmg = resolve_attack(cmd, enemy, 0);
        m_view.enemy_hp = (dmg >= m_view.enemy_hp) ? 0 : m_view.enemy_hp - dmg;
        NARR("Zero: %s strikes for %u damage", cmd.name.c_str(), dmg);
        if (m_view.enemy_hp == 0) {
            on_victory(log);
            return log;
        }
    }

    // ---- enemy reply ----
    uint32_t dmg = 0;
    if (m_guard_next > 0) {
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
        dmg = std::max<uint32_t>(1, enemy.str + 2 - player_def(0) / 2);
        NARR("%s strikes for %u damage.", enemy.name.c_str(), dmg);
    }
    rec.hp = (dmg >= rec.hp) ? 0 : rec.hp - dmg;
    m_view.hp = rec.hp;
    if (rec.hp == 0) {
        NARR("[The dark closes around Zero...]");
        m_view.phase = BattlePhase::Defeat;
        m_result = BattleResult{};
        m_result.victory = false;
        return log;
    }
    m_view.phase = BattlePhase::PlayerTurn;
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
    m_view.hp = rec.hp;
    m_view.mp = rec.mp;
    rebuild_deck();
    m_view.deck = m_deck;
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
        std::printf("\n  \x1b[38;5;196m%s\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u\n",
                    m_view.enemy_name.c_str(),
                    hp_bar(m_view.enemy_hp, m_view.enemy_max_hp, 24).c_str(),
                    m_view.enemy_hp, m_view.enemy_max_hp);
        std::printf("  \x1b[1mZero\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u  MP \x1b[38;5;27m%s\x1b[0m %u/%u\n",
                    hp_bar(m_view.hp, m_view.max_hp, 24).c_str(), m_view.hp, m_view.max_hp,
                    hp_bar(m_view.mp, m_view.max_mp, 16).c_str(), m_view.mp, m_view.max_mp);
        std::printf("  \x1b[2m[deck]\x1b[0m ");
        for (size_t i = 0; i < m_deck.size(); ++i)
            std::printf("%zu) %s\x1b[2m(%uMP)\x1b[0m  ", i + 1,
                        m_deck[i].name.c_str(), m_deck[i].cost);
        std::printf("\n  \x1b[2m[action]\x1b[0m ");
        std::string line;
        if (!std::getline(std::cin, line)) { m_result.escaped = true; return m_result; }
        int choice = std::atoi(line.c_str());
        if (choice < 1 || (size_t)choice > m_deck.size()) {
            std::printf("  \x1b[2m(the command is forgotten - the dark murmurs)\x1b[0m\n");
            continue;
        }
        act((size_t)choice - 1);
    }

    if (m_view.phase == BattlePhase::Victory) {
        std::printf("\n  \x1b[38;5;220m[%s falls.]\x1b[0m\n", m_view.enemy_name.c_str());
    } else if (m_view.phase == BattlePhase::Defeat) {
        std::printf("\n  \x1b[38;5;196m[The dark closes around Zero...]\x1b[0m\n");
    }
    return m_result;
}

} // namespace khz