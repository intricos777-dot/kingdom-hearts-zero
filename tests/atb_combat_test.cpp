#include "combat/combat.h"
#include "data/enemies.h"
#include "data/keyblades.h"
#include "crafting/materials.h"
#include "save/save_system.h"
#include <cassert>
#include <cstdio>
#include <string>

// FF7R-flavored combat checks: ATB gating, pressure -> stagger (1.5x in,
// counter interrupted), free dodge/block, and the Echo leader swap.
// The engine never reads a wall clock here - hosts feed tick_atb(dt).

namespace {
const char* KEYB = "/home/sin/Projects/games/kingdom-hearts-zero/data/combat/keyblades.json";
const char* MATS = "/home/sin/Projects/games/kingdom-hearts-zero/data/crafting/material_catalog.json";

khz::EnemyDef foe(const char* id, const char* element, uint32_t hp,
                  uint32_t str = 0, uint32_t stagger = 60) {
    khz::EnemyDef e;
    e.id = id;
    e.name = id;
    e.kind = "shambler";   // boss-shaped, but no attacks: deterministic reply
    e.world = "test";
    e.act = 1;
    e.hp = hp;
    e.str = str;
    e.mag = 0;
    e.def = 0;
    e.spd = 1;
    e.crt = 0;
    e.exp = 5;
    e.element = element;   // "blizzard" => fire is the weakness (x1.5)
    e.stagger = stagger;
    return e;
}

size_t card(const khz::BattleView& v, const char* element) {
    for (size_t i = 0; i < v.deck.size(); ++i)
        if (v.deck[i].element == element) return i;
    return 0;
}

bool has_line(const std::vector<std::string>& lines, const char* needle) {
    for (const auto& l : lines)
        if (l.find(needle) != std::string::npos) return true;
    return false;
}
}

int main() {
    khz::SaveSystem saves;
    if (!saves.initialize()) { std::printf("FAIL: saves init\n"); return 1; }
    khz::KeybladeDB blades;
    if (!blades.load(KEYB)) { std::printf("FAIL: keyblades.json\n"); return 1; }
    khz::MaterialCatalog mats;
    if (!mats.load(MATS)) { std::printf("FAIL: materials\n"); return 1; }

    khz::CombatEngine engine;
    engine.bind(saves, blades, &mats);
    auto& rec = saves.record();
    rec.base_crt = 0;          // kill crit RNG: damage math is exact
    rec.level = 5;             // Fira available (slot 1)
    rec.hp = rec.max_hp = 999;

    // ---- 1. ATB gates commands; a spell costs a full segment ----
    {
        engine.begin(foe("gate_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        assert(v.leader_is_zero && v.echo_in);
        assert(v.atb >= 1.0f);
        size_t fira = 1;  // level-5 deck: 0 Slash, 1 Fira
        assert(engine.atb_ready(fira));
        uint32_t before = v.enemy_hp;
        uint32_t hp0 = v.hp;
        engine.act(fira);                 // cast: gauge deltas
        assert(v.atb < 0.5f);
        assert(v.enemy_hp < before);      // hit landed
        assert(v.hp < hp0);               // the reply answered
        // immediately re-casting is refused: no damage, no reply
        uint32_t mid = v.enemy_hp;
        uint32_t hp1 = v.hp;
        auto lines = engine.act(fira);
        assert(has_line(lines, "gauge"));
        assert(v.enemy_hp == mid && v.hp == hp1);
        // the gauge charges when the host feeds time
        for (int i = 0; i < 50 && v.atb < 1.0f; ++i) engine.tick_atb(0.1f);
        assert(v.atb >= 1.0f);
        std::printf("[atb] gate + charge OK\n");
    }

    // ---- 2. weakness presses pressure; stagger interrupts + 1.5x ----
    {
        engine.begin(foe("stagger_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        engine.tick_atb(1.0f);  // start full
        engine.act(1);          // weak hit #1: pressure 30
        assert(!v.staggered && v.stagger >= 30.0f);
        engine.tick_atb(1.0f);
        uint32_t hpA = v.hp;
        engine.act(1);          // weak hit #2: 60 >= 60 -> STAGGER
        assert(v.staggered);
        assert(v.hp == hpA);    // the broken counter never answered
        // staggered target eats 1.5x: Fira(lv5)=26+5 mag, weak x1.5, stagger x1.5
        engine.tick_atb(1.0f);
        uint32_t hpB = v.enemy_hp;
        engine.act(1);
        uint32_t delta = hpB - v.enemy_hp;
        assert(delta >= 60u);   // 46 base weak -> ~69 staggered
        assert(v.hp == hpA);    // still interrupted
        engine.tick_atb(2.1f);  // stagger drains
        assert(!v.staggered);
        std::printf("[stagger] press/interrupt/1.5x/drain OK\n");
    }

    // ---- 3. basic swings are free and build the bar ----
    {
        engine.begin(foe("slash_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        assert(v.atb == 1.0f);
        engine.act(0);                      // slash: free
        assert(v.atb > 1.1f && v.atb < 1.2f);  // +0.12 toward the next cast
        std::printf("[slash] free + charges OK\n");
    }

    // ---- 4. guard blocks the reply; dodge slips it ----
    {
        engine.begin(foe("guard_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        uint32_t h = v.hp;
        size_t gi = card(v, "guard");
        assert(v.atb >= v.atb_cost(gi));    // 1.0 >= 0.5
        engine.act(gi);
        assert(v.hp == h);                  // blocked
    }
    {
        engine.begin(foe("dodge_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        uint32_t h = v.hp;
        engine.dodge_up();
        engine.act(0);                      // slash; reply
        assert(v.hp == h);                  // slipped
        std::printf("[defense] guard + dodge OK\n");
    }

    // ---- 5. the Echo: swap leader, own pool, own deck ----
    {
        engine.begin(foe("echo_test", "blizzard", 100000));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        auto sw = engine.swap_leader();
        assert(v.leader_is_zero == false);
        assert(has_line(sw, "Echo"));
        assert(v.echo_hp == v.echo_max_hp);
        assert(v.hp == v.echo_hp);          // HUD mirrors the leader
        assert(v.deck.size() == v.partner_deck.size());
        assert(v.atb < 1.0f);               // swap cost a segment
        engine.tick_atb(1.0f);
        uint32_t e_mp = v.echo_mp;
        uint32_t e_hp = v.echo_hp;
        engine.act(1);                      // Echo Fira
        assert(v.echo_mp < e_mp);           // from its own pool
        assert(v.echo_hp < e_hp);           // the reply landed on Echo
        std::printf("[echo] swap + pool + deck OK\n");
    }

    // ---- 6. the Echo unravels instead of dying; the fight returns to Zero ----
    {
        engine.begin(foe("ko_test", "blizzard", 100000, /*str=*/300));
        engine.select_form(0);
        const khz::BattleView& v = engine.view();
        engine.swap_leader();               // Echo leads (atb 1.0)
        engine.tick_atb(1.0f);
        uint32_t zero_hp = v.hp;
        engine.act(0);                      // Echo slash; ~297 dmg reply
        assert(v.echo_in == false);
        assert(v.leader_is_zero == true);   // handed back to Zero
        assert(v.hp == zero_hp);            // Zero never took the hit
        auto refused = engine.swap_leader();
        assert(has_line(refused, "unraveled"));
        assert(v.leader_is_zero == true);
        std::printf("[echo] KO + auto-return OK\n");
    }

    std::printf("PASS\n");
    return 0;
}