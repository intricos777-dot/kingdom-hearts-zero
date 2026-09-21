#include "crafting/moogle_stall.h"
#include "data/keyblades.h"
#include "save/save_system.h"
#include <cassert>
#include <cstdio>

// The bazaar's headless check: every blueprint on the shelf must resolve,
// the knack of forging Ultima Weapon must pay the dark's dues, and the story
// gate must keep the forge shut until the arc arrives.
int main() {
    khz::SaveSystem saves;
    if (!saves.initialize()) return 1;

    khz::MoogleStall stall;
    if (!stall.load("/home/sin/Projects/games/kingdom-hearts-zero/data/crafting/material_catalog.json",
                    "/home/sin/Projects/games/kingdom-hearts-zero/data/crafting/recipes.json",
                    "/home/sin/Projects/games/kingdom-hearts-zero/data/crafting/moogle_stall.json")) {
        std::printf("FAIL: stall load\n");
        return 1;
    }

    khz::KeybladeDB blades;
    if (!blades.load("/home/sin/Projects/games/kingdom-hearts-zero/data/combat/keyblades.json")) {
        std::printf("FAIL: keyblades.json\n");
        return 1;
    }

    // every keyblade blueprint must resolve to a real blade in the registry
    for (const auto& r : stall.recipes()) {
        if (r.kind == "keyblade") {
            assert(blades.index_for_id(r.id) >= 0);
        }
    }

    // citadels of the recipe book
    const khz::Recipe* ultima = nullptr;
    const khz::Recipe* keyseal = nullptr;
    const khz::Recipe* twilight_form = nullptr;
    for (const auto& r : stall.recipes()) {
        if (r.id == "ultima_weapon") ultima = &r;
        if (r.id == "ultima_keyseal") keyseal = &r;
        if (r.id == "twilight_form") twilight_form = &r;
    }
    assert(ultima && keyseal && twilight_form);
    assert(ultima->arc == 3 && ultima->cost > 0 && !ultima->mats.empty());
    assert(keyseal->cost > ultima->cost);          // the final forge is priciest
    assert(keyseal->arc == 5);

    auto& rec = saves.record();

    // 1) the arc gate: before the story arrives, the forge refuses anything
    {
        std::string flavor;
        auto res = stall.craft(*ultima, rec, blades, flavor);
        assert(res == khz::MoogleStall::CraftResult::Locked);
    }

    // 2) the arc arrives (endgame); the forge demands munny + the dark's dues
    rec.story_progress = 5;
    {
        std::string flavor;
        auto res = stall.craft(*ultima, rec, blades, flavor);
        assert(res == khz::MoogleStall::CraftResult::NoMunny);
    }
    rec.munny = 50000;

    // find the Ultima Weapon's material indexes and satisfy the recipes by hand
    auto satisfy = [&](const khz::Recipe& r) {
        rec.munny = 50000;
        for (const auto& miq : r.mats)
            rec.materials[miq.first] += miq.second + 5;
    };

    // first forge Shadow Overdrive (an early-branch recipe) - proves grant path
    const khz::Recipe* shadow = nullptr;
    for (const auto& r : stall.recipes())
        if (r.id == "shadow_overdrive") shadow = &r;
    assert(shadow);
    satisfy(*shadow);
    {
        std::string flavor;
        auto res = stall.craft(*shadow, rec, blades, flavor);
        assert(res == khz::MoogleStall::CraftResult::Ok);
        assert(rec.forms_unlocked & FORM_SHADOW);
        // already owned: refuses before spending
        auto res2 = stall.craft(*shadow, rec, blades, flavor);
        assert(res2 == khz::MoogleStall::CraftResult::AlreadyOwned);
    }

    // Ultima Weapon: munny + materials consumed, blade granted + equipped
    satisfy(*ultima);
    {
        std::string flavor;
        auto res = stall.craft(*ultima, rec, blades, flavor);
        assert(res == khz::MoogleStall::CraftResult::Ok);
        int32_t ui = blades.index_for_id("ultima_weapon");
        assert(ui > 0 && (rec.owned_keyblades & (1u << ui)));
        assert(rec.active_keyblade == (uint32_t)ui);   // first forged blade to hand
        // the recipe paid its dues
        for (const auto& miq : ultima->mats)
            assert(rec.materials[miq.first] == 0);     // consumed exactly
        assert(rec.munny == 50000 - ultima->cost);

        // no double-forge
        satisfy(*ultima);
        auto res2 = stall.craft(*ultima, rec, blades, flavor);
        assert(res2 == khz::MoogleStall::CraftResult::AlreadyOwned);
    }

    // Ultima Keyseal: First Light must come from the end of the road
    satisfy(*keyseal);
    {
        std::string flavor;
        auto res = stall.craft(*keyseal, rec, blades, flavor);
        assert(res == khz::MoogleStall::CraftResult::Ok);
        int32_t ki = blades.index_for_id("ultima_keyseal");
        assert(ki > 0 && (rec.owned_keyblades & (1u << ki)));
    }

    printf("PASS\n");
    return 0;
}