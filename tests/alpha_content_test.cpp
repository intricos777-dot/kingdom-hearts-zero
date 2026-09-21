#include "world/npc.h"
#include "world/areas.h"
#include "combat/drive_forms.h"
#include "combat/door_seal.h"
#include <cstdio>
#include <cassert>

// Smoke test: load the new alpha content (NPCs, areas, drive forms, doors).
int main() {
    khz::NPCDb npcs;
    if (!npcs.load("/home/sin/Projects/games/kingdom-hearts-zero/data/worlds/npcs.json")) {
        std::printf("FAIL: npcs.json\n");
        return 1;
    }
    std::printf("[npc] loaded %zu characters\n", npcs.npcs().size());

    // Sora should exist and have a schedule.
    const khz::NPCDef* sora = npcs.find("sora");
    assert(sora);
    khz::NPCClock clock;
    clock.set_hour(12);
    std::string line = clock.present(npcs, *sora, "destiny_islands");
    std::printf("[npc] sora@destiny_islands h12: %s\n", line.c_str());

    // NPCs in Traverse Town.
    auto tt = npcs.in_world("traverse_town");
    std::printf("[npc] traverse_town has %zu NPCs\n", tt.size());

    khz::AreaDb areas;
    if (!areas.load(
            "/home/sin/Projects/games/kingdom-hearts-zero/data/worlds/kh1_areas.json",
            "/home/sin/Projects/games/kingdom-hearts-zero/data/worlds/kh2_areas.json")) {
        std::printf("FAIL: area manifests\n");
        return 1;
    }
    std::printf("[areas] loaded %zu areas\n", areas.areas().size());

    // Traverse Town should have three areas.
    auto tt_areas = areas.in_world("traverse_town");
    std::printf("[areas] traverse_town has %zu areas\n", tt_areas.size());
    for (auto a : tt_areas) std::printf("   - %s: %s\n", a->id.c_str(), a->name.c_str());

    khz::DriveFormDb forms;
    if (!forms.load("/home/sin/Projects/games/kingdom-hearts-zero/data/combat/drive_forms.json")) {
        std::printf("FAIL: drive_forms.json\n");
        return 1;
    }
    std::printf("[drive_forms] loaded %zu forms\n", forms.forms().size());
    for (const auto& f : forms.forms())
        std::printf("   - %s (bit %u, unlock@mission %u)\n", f.name.c_str(), f.bit, f.unlock_story_progress);

    // forms available after mission 3
    auto avail = forms.available(3);
    std::printf("[drive_forms] available at story_progress=3: %zu\n", avail.size());

    khz::DoorSealLedger doors;
    if (!doors.load("/home/sin/Projects/games/kingdom-hearts-zero/data/worlds/doors.json")) {
        std::printf("FAIL: doors.json\n");
        return 1;
    }
    std::printf("[doors] loaded %zu doors\n", doors.total_count());
    doors.keyseals_available = 3;

    // Seal first door (Traverse Town).
    doors.seal("door_traverse_town");
    std::printf("[doors] sealed %zu / %zu (%.0f%%)\n", doors.sealed_count(), doors.total_count(), doors.progress() * 100.0f);

    // Keyseal a second door.
    doors.keyseal("door_destiny_islands");
    std::printf("[doors] sealed %zu / %zu; keyseals left: %u\n", doors.sealed_count(), doors.total_count(), doors.keyseals_available);

    const khz::DoorDef* finaldoor = doors.final_door();
    if (finaldoor) std::printf("[doors] final door: %s (act %u, keyseal allowed: %s)\n",
                                finaldoor->name.c_str(), finaldoor->act, finaldoor->can_keyseal ? "yes" : "no");

    std::printf("PASS\n");
    return 0;
}
