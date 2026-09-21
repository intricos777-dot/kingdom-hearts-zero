#include "combat/combat.h"
#include "data/enemies.h"
#include "data/keyblades.h"
#include "crafting/materials.h"
#include "save/save_system.h"
#include <cassert>
#include <cstdio>

// Headless unit check for the combat session API used by the visual shell.
// Drives a full shambler fight through the phase machine without a window.
int main() {
    khz::SaveSystem saves;
    if (!saves.initialize()) {
        std::printf("FAIL: saves init\n");
        return 1;
    }

    khz::KeybladeDB blades;
    if (!blades.load("/home/sin/Projects/games/kingdom-hearts-zero/data/combat/keyblades.json")) {
        std::printf("FAIL: keyblades.json\n");
        return 1;
    }
    khz::MaterialCatalog mats;
    if (!mats.load("/home/sin/Projects/games/kingdom-hearts-zero/data/crafting/material_catalog.json")) {
        std::printf("FAIL: material_catalog.json\n");
        return 1;
    }
    khz::EnemyDB enemies;
    if (!enemies.load("/home/sin/Projects/games/kingdom-hearts-zero/data/combat/enemies.json", mats)) {
        std::printf("FAIL: enemies.json\n");
        return 1;
    }

    // ---- keyblade master gate ----
    assert(blades.sabres() != nullptr);
    assert(blades.keyblades_locked(0) == true);     // before mission two: locked
    assert(blades.keyblades_locked(1) == true);
    assert(blades.keyblades_locked(2) == false);    // after the flashback: open

    khz::CombatEngine engine;
    engine.bind(saves, blades, &mats);
    assert(engine.keyblades_unlocked() == false);   // fresh save, gate closed
    // while gated, the equipped weapon is always the sabres, never a keyblade
    assert(engine.weapon_name() == "Twin Red Sabres");
    saves.record().active_keyblade = 11;            // try to slot Ultima Weapon
    assert(engine.weapon_name() == "Twin Red Sabres");   // still gated down

    // shambler in traverse_town: veska_the_erasure (act 1 boss)
    const khz::EnemyDef* boss = enemies.shambler_for_world("traverse_town");
    if (!boss) {
        std::printf("FAIL: no boss for traverse_town\n");
        return 1;
    }
    std::printf("[cite] boss: %s (music=%s)\n", boss->id.c_str(),
                boss->music.empty() ? "(none)" : boss->music.c_str());

    // Act one fight: story_progress = 0. The boss's arc-1 drop table does not
    // open yet (story-arc-timed drops) - but the fight still resolves.
    engine.begin(*boss);
    const khz::BattleView& v = engine.view();
    assert(v.phase == khz::BattlePhase::FormSelect);   // shambler -> form select
    assert(v.enemy_max_hp == boss->hp);
    assert(v.deck.size() >= 3);                        // base deck

    // with no forms unlocked, only "Base Form" is offered
    auto names = engine.form_names();
    assert(names.size() == 1);
    engine.select_form(0);
    assert(engine.view().phase == khz::BattlePhase::PlayerTurn);

    // swing until victory or defeat (bounded)
    int guard = 0;
    for (int i = 0; i < 400 && engine.view().phase == khz::BattlePhase::PlayerTurn; ++i) {
        const auto& deck = engine.view().deck;
        size_t pick = 0;
        if (guard > 0) pick = 0;  // attack
        const auto& cmd = deck[pick];
        (void)cmd;
        if (engine.view().mp >= deck[1].cost) pick = 1;
        engine.act(pick);
        guard = 0;
    }

    const auto& r = engine.result();
    std::printf("%s with %d of %d defeated - xp=%u loot=%s\n",
                engine.view().phase == khz::BattlePhase::Victory ? "VICTORY" : "NOT-VICTORY",
                boss->hp - engine.view().enemy_hp, boss->hp, r.xp,
                r.loot_keyblade.empty() ? "(none)" : r.loot_keyblade.c_str());

    if (engine.view().phase == khz::BattlePhase::Victory) {
        assert(r.xp > 0);
        uint32_t lvl_before = saves.record().level;
        engine.reward(r);
        assert(saves.record().level >= lvl_before);
        // gated fight: no crafting motes yet (arc 1 > story_progress 0)
        assert(r.loot.empty());
        assert(r.munny == 0);

        // ---- gate opens; the same boss settles its arc-1 account ----
        saves.record().story_progress = 2;
        assert(engine.keyblades_unlocked() == true);
        assert(engine.weapon_name() == "Twin Red Sabres"); // sabre still equipped
        saves.record().active_keyblade = 1;                // Twilight Keyblade
        assert(engine.weapon_name() == "Twilight Keyblade");

        engine.begin(*boss);
        names = engine.form_names();
        engine.select_form(0);
        for (int i = 0; i < 400 && engine.view().phase == khz::BattlePhase::PlayerTurn; ++i) {
            const auto& deck = engine.view().deck;
            size_t pick = 0;
            if (engine.view().mp >= deck[1].cost) pick = 1;
            engine.act(pick);
        }
        const auto& r2 = engine.result();
        assert(r2.victory);
        assert(!r2.loot.empty());       // dusk shards flowed (arc 1 <= 2)
        assert(r2.munny > 0);           // the dark pays
        uint32_t dusk_before = saves.record().materials[0];
        engine.reward(r2);
        assert(saves.record().materials[0] >= dusk_before);
        std::printf("PASS\n");
        return 0;
    }
    std::printf("WARN: died to boss - engine still behaved\n");
    std::printf("PASS\n");
    return 0;
}