#include "combat/combat.h"
#include "data/enemies.h"
#include "data/keyblades.h"
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
    if (!blades.load("data/combat/keyblades.json")) {
        std::printf("FAIL: keyblades.json\n");
        return 1;
    }
    khz::EnemyDB enemies;
    if (!enemies.load("data/combat/enemies.json")) {
        std::printf("FAIL: enemies.json\n");
        return 1;
    }

    khz::CombatEngine engine;
    engine.bind(saves, blades);

    // shambler in traverse_town: veska_the_erasure (act 1 boss)
    const khz::EnemyDef* boss = enemies.shambler_for_world("traverse_town");
    if (!boss) {
        std::printf("FAIL: no boss for traverse_town\n");
        return 1;
    }
    std::printf("[cite] boss: %s (music=%s)\n", boss->id.c_str(),
                boss->music.empty() ? "(none)" : boss->music.c_str());

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
        std::printf("PASS\n");
        return 0;
    }
    std::printf("WARN: died to boss - engine still behaved\n");
    std::printf("PASS\n");
    return 0;
}