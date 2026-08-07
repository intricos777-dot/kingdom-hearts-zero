#include "combat/combat.h"
#include "ui/command_deck.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>

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
    : m_saves(saves), m_blades(blades) {
    std::srand((unsigned)std::time(nullptr));
    rebuild_deck();
}

// ---- deck assembly: KH2-style unlocks by level, themed by the world ----

void CombatEngine::rebuild_deck() {
    m_deck.clear();
    const uint32_t lvl = m_saves.record().level;
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

// Twilight form: stats from up to 5 owned blades at once.
struct SummedBlade {
    uint32_t str = 0, mag = 0, def = 0, crt = 0, spd = 0;
    uint32_t count = 0;
};
SummedBlade sum_blades(const SaveRecord& rec, const KeybladeDB& db, uint32_t limit) {
    SummedBlade s;
    std::vector<const KeybladeDef*> owned;
    for (uint32_t i = 0; i < 32; ++i) {
        if (rec.owned_keyblades & (1u << i))
            if (const KeybladeDef* b = db.by_index(i)) owned.push_back(b);
    }
    std::sort(owned.begin(), owned.end(), [](const KeybladeDef* a, const KeybladeDef* b) {
        return (a->str + a->mag) > (b->str + b->mag);
    });
    size_t n = std::min<size_t>(limit, owned.size());
    for (size_t i = 0; i < n; ++i) {
        s.str += owned[i]->str;
        s.mag += owned[i]->mag;
        s.def += owned[i]->def;
        s.crt += owned[i]->crt;
        s.spd += (uint32_t)std::max(0, owned[i]->spd);
        ++s.count;
    }
    return s;
}
}

uint32_t CombatEngine::player_str(uint32_t idx) {
    const SaveRecord& rec = m_saves.record();
    uint32_t base = rec.base_str;
    if (m_form == FORM_TWILIGHT) {
        base += sum_blades(rec, m_blades, 5).str;
        if (m_form & FORM_SHADOW) base = base * 3 / 2;
        return base;
    }
    const KeybladeDef* b = active_blade(rec, m_blades);
    if (b) base += b->str;
    if (m_form == FORM_SHADOW) base = base * 3 / 2;
    return base;
}

uint32_t CombatEngine::player_mag(uint32_t idx) {
    const SaveRecord& rec = m_saves.record();
    uint32_t base = rec.base_mag;
    if (m_form == FORM_TWILIGHT) {
        base += sum_blades(rec, m_blades, 5).mag;
        if (m_form & FORM_ULTIMA) base = base * 3 / 2;
        return base;
    }
    const KeybladeDef* b = active_blade(rec, m_blades);
    if (b) base += b->mag;
    if (m_form == FORM_ULTIMA) base = base * 3 / 2;
    return base;
}

uint32_t CombatEngine::player_def(uint32_t idx) {
    const SaveRecord& rec = m_saves.record();
    uint32_t base = rec.base_def;
    if (m_form == FORM_TWILIGHT) {
        base += sum_blades(rec, m_blades, 5).def;
        return base;
    }
    const KeybladeDef* b = active_blade(rec, m_blades);
    if (b) base += b->def;
    return base;
}

uint32_t CombatEngine::player_crt(uint32_t idx) {
    const SaveRecord& rec = m_saves.record();
    uint32_t base = rec.base_crt;
    if (m_form == FORM_TWILIGHT) {
        base += sum_blades(rec, m_blades, 5).crt;
        return base;
    }
    const KeybladeDef* b = active_blade(rec, m_blades);
    if (b) base += b->crt;
    return base;
}

int32_t CombatEngine::player_spd(uint32_t idx) {
    const SaveRecord& rec = m_saves.record();
    int32_t base = (int32_t)rec.base_spd;
    if (m_form == FORM_TWILIGHT) {
        base += (int32_t)sum_blades(rec, m_blades, 5).spd;
        return base;
    }
    const KeybladeDef* b = active_blade(rec, m_blades);
    if (b) base += b->spd;
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
    auto& rec = m_saves.record();
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

    // form bonuses
    if (m_form & FORM_SHADOW && cmd.element == "dark")
        dmg = (int32_t)((uint32_t)dmg * 130 / 100);
    if (m_form & FORM_ULTIMA && (cmd.element == "fire" || cmd.element == "blizzard" ||
                                 cmd.element == "thunder"))
        dmg = (int32_t)((uint32_t)dmg * 130 / 100);

    std::printf("  \x1b[1mZero\x1b[0m uses \x1b[1m%s\x1b[0m", cmd.name.c_str());
    if (crit) std::printf(" - \x1b[38;5;220mCRITICAL\x1b[0m");
    std::printf(" - \x1b[38;5;196m%d\x1b[0m damage.\n", dmg);
    return (uint32_t)std::max(0, dmg);
}

uint32_t CombatEngine::enemy_attack_damage(const EnemyAttack& atk, uint32_t& memories) {
    auto& rec = m_saves.record();
    uint32_t stat = (atk.element == "dark" || atk.element == "void")
                        ? (atk.power + 6) * 1 : atk.power;
    int32_t dmg = (int32_t)atk.power + (int32_t)(rec.hp > 0 ? 0 : 0);
    dmg = (int32_t)((uint32_t)atk.power + (atk.element == "void" ? 12 : 4));
    dmg = std::max<int32_t>(0, (int32_t)((uint32_t)dmg * element_multiplier(atk.element, "none") / 100) -
                                  (int32_t)player_def(0) / 2);
    if (dmg < 1) dmg = 1;

    if (atk.effect == "steal" && rec.memories_held > 0) {
        --rec.memories_held;
        ++memories;
        std::printf("  \x1b[38;5;196m%s\x1b[0m \x1b[2msteals a memory from you! (%u left)\x1b[0m\n",
                    atk.name.c_str(), rec.memories_held);
        return 0;
    }
    if (atk.effect == "debuff") {
        std::printf("  \x1b[38;5;196m%s\x1b[0m - the dark weighs on you.\n", atk.name.c_str());
    }
    std::printf("  \x1b[38;5;196m%s\x1b[0m hits for \x1b[38;5;208m%d\x1b[0m.\n",
                atk.name.c_str(), dmg);
    return (uint32_t)dmg;
}

// ---- the battle loop ----

BattleResult CombatEngine::battle(const EnemyDef& enemy) {
    auto& rec = m_saves.record();
    BattleResult res;
    std::printf("\n\x1b[1m\x1b[38;5;196m  %s\x1b[0m  \x1b[2m(%s)\x1b[0m\n",
                enemy.name.c_str(), enemy.kind == "shambler" ? "SHAMBLER" : "HEARTLESS");
    if (!enemy.desc.empty())
        std::printf("  \x1b[2m%s\x1b[0m\n\n", enemy.desc.c_str());

    uint32_t ehp = enemy.hp;
    const bool boss = (enemy.kind == "shambler");
    uint32_t memories = 0;

    // form select (bosses only)
    if (boss) {
        std::printf("  \x1b[1mChoose your form:\x1b[0m\n");
        std::printf("   1) base        - no bonus\n");
        if (rec.forms_unlocked & FORM_SHADOW)
            std::printf("   2) shadow      - SHADOW OVERDRIVE (heartless form)\n");
        if (rec.forms_unlocked & FORM_ULTIMA)
            std::printf("   3) ultima      - ULTIMA DRIVE\n");
        if (rec.forms_unlocked & FORM_TWILIGHT)
            std::printf("   4) twilight    - five keyblades at once\n");
        std::printf("  \x1b[2m[form]\x1b[0m ");
        char buf[16];
        if (std::fgets(buf, sizeof(buf), stdin)) {
            switch (buf[0]) {
                case '2': if (rec.forms_unlocked & FORM_SHADOW) m_form = FORM_SHADOW; break;
                case '3': if (rec.forms_unlocked & FORM_ULTIMA) m_form = FORM_ULTIMA; break;
                case '4': if (rec.forms_unlocked & FORM_TWILIGHT) m_form = FORM_TWILIGHT; break;
                default: m_form = 0;
            }
        } else {
            m_form = 0;
        }
        if (m_form)
            std::printf("  \x1b[2m(form engaged)\x1b[0m\n");
        rebuild_deck();
    } else {
        m_form = 0;
    }

    uint32_t guard_next = 0;
    bool player_turn_first = (player_spd(0) >= enemy.spd);

    while (rec.hp > 0 && ehp > 0) {
        // ---- player turn ----
        std::printf("\n  \x1b[38;5;196m%s\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u\n",
                    enemy.name.c_str(), hp_bar(ehp, enemy.hp, 24).c_str(), ehp, enemy.hp);
        std::printf("  \x1b[1mZero\x1b[0m HP \x1b[38;5;28m%s\x1b[0m %u/%u  MP \x1b[38;5;27m%s\x1b[0m %u/%u\n",
                    hp_bar(rec.hp, rec.max_hp, 24).c_str(), rec.hp, rec.max_hp,
                    hp_bar(rec.mp, rec.max_mp, 16).c_str(), rec.mp, rec.max_mp);
        std::printf("  \x1b[2m[deck]\x1b[0m ");
        for (size_t i = 0; i < m_deck.size(); ++i) {
            const auto& c = m_deck[i];
            std::printf("%zu) %s\x1b[2m(%uMP)\x1b[0m  ", i + 1, c.name.c_str(), c.cost);
        }
        std::printf("\n  \x1b[2m[action]\x1b[0m ");
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) { res.victory = false; return res; }
        int choice = std::atoi(buf);
        if (choice < 1 || (size_t)choice > m_deck.size()) {
            std::printf("  \x1b[2m(the command is forgotten - the dark murmurs)\x1b[0m\n");
            continue;
        }
        const Command& cmd = m_deck[choice - 1];

        if (cmd.element == "cure") {
            uint32_t heal = cmd.power;
            uint32_t old = rec.hp;
            rec.hp = std::min(rec.max_hp, rec.hp + heal);
            std::printf("  \x1b[1mZero\x1b[0m casts Cura - restored \x1b[38;5;46m%u\x1b[0m HP.\n",
                        rec.hp - old);
        } else if (cmd.element == "focus") {
            uint32_t gain = cmd.power;
            rec.mp = std::min(rec.max_mp, rec.mp + gain);
            std::printf("  \x1b[1mZero\x1b[0m focuses - recovered \x1b[38;5;51m%u\x1b[0m MP.\n", gain);
        } else if (cmd.element == "guard") {
            guard_next = player_def(0) * 2 + 8;
            std::printf("  \x1b[1mZero\x1b[0m raises the guard.\n");
        } else {
            if (rec.mp < cmd.cost) {
                std::printf("  \x1b[2m(not enough MP - the command card flickers)\x1b[0m\n");
                continue;
            }
            rec.mp -= cmd.cost;
            uint32_t dmg = resolve_attack(cmd, enemy, 0);
            if (dmg >= ehp) ehp = 0;
            else ehp -= dmg;
        }

        if (ehp == 0) break;

        // ---- enemy turn ----
        uint32_t dmg = 0;
        if (guard_next > 0) {
            std::printf("  \x1b[38;5;196m%s\x1b[0m strikes the guard - \x1b[2mblocked\x1b[0m.\n",
                        enemy.name.c_str());
            guard_next = 0;
        } else if (!enemy.attacks.empty() && (boss || (std::rand() % 3) == 0)) {
            const EnemyAttack& atk = enemy.attacks[std::rand() % enemy.attacks.size()];
            dmg = enemy_attack_damage(atk, memories);
        } else {
            dmg = std::max<uint32_t>(1, enemy.str + 2 - player_def(0) / 2);
            std::printf("  \x1b[38;5;196m%s\x1b[0m attacks for \x1b[38;5;208m%u\x1b[0m.\n",
                        enemy.name.c_str(), dmg);
        }
        if (dmg >= rec.hp) rec.hp = 0;
        else rec.hp -= dmg;

        if (rec.hp == 0) {
            std::printf("\n  \x1b[38;5;196m[The dark closes around Zero...]\x1b[0m\n");
            res.victory = false;
            return res;
        }
    }

    if (ehp == 0) {
        res.victory = true;
        res.xp = enemy.exp * (boss ? 2 : 1);
        res.memories_stolen = memories;
        std::printf("\n  \x1b[38;5;220m[%s falls.]\x1b[0m  \x1b[1m+%u XP\x1b[0m\n",
                    enemy.name.c_str(), res.xp);
        if (boss && !enemy.loot_keyblade.empty() && enemy.loot_keyblade != "luxords_die") {
            uint32_t bit = 0;
            for (size_t i = 0; i < m_blades.all().size(); ++i) {
                if (m_blades.all()[i].id == enemy.loot_keyblade) { bit = (1u << i); break; }
            }
            if (bit && !(rec.owned_keyblades & bit)) {
                rec.owned_keyblades |= bit;
                res.loot_keyblade = enemy.loot_keyblade;
                res.loot_desc = enemy.loot_desc;
            }
        }
    }
    return res;
}

// ---- XP + leveling (KH2-style growth) ----

void CombatEngine::reward(const BattleResult& r) {
    auto& rec = m_saves.record();
    if (!r.victory) return;
    rec.xp += r.xp;
    std::vector<uint32_t> gained;
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
        gained.push_back(rec.level);
    }
    for (uint32_t lvl : gained) {
        std::printf("  \x1b[38;5;220m\x1b[1m[LEVEL UP] Zero is now level %u\x1b[0m\n", lvl);
        std::printf("  \x1b[2m  HP +6  MP +3  and the command deck grows\x1b[0m\n");
    }
    rebuild_deck();
}

bool CombatEngine::unlock_form(uint32_t form_bit, const std::string& flavor) {
    auto& rec = m_saves.record();
    if (rec.forms_unlocked & form_bit) return true;
    rec.forms_unlocked |= form_bit;
    std::printf("\n  \x1b[38;5;141m\x1b[1m[A new form awakens]\x1b[0m  %s\n", flavor.c_str());
    return true;
}

} // namespace khz