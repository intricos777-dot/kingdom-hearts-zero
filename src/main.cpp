#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "story/scene.h"
#include "ui/command_deck.h"
#include "save/save_system.h"
#include "ui/tron_shell.h"
#include "render/worlds.h"
#include "data/keyblades.h"
#include "crafting/materials.h"
#include "crafting/moogle_stall.h"

namespace {

const char* INTRO_VIDEO = "assets/intro/intro.mp4";

void play_flashback_with_sora(khz::SaveSystem& saves, const khz::KeybladeDB& blades);
int act_two(khz::SaveSystem& saves, const khz::KeybladeDB& blades);
int moogle_flow(khz::SaveSystem& saves, khz::MoogleStall& stall,
                const khz::KeybladeDB& blades);

void title_screen() {
    std::printf("\x1b[38;5;141m\x1b[1m");
    std::printf("  KINGDOM HEARTS 0: DOOR TO DARKNESS\n");
    std::printf("  ==================================\n");
    std::printf("\x1b[0m");
    std::printf("  \x1b[2mAn Ansem-narrated terminal adventure built on Twilight Elysium\x1b[0m\n\n");
}

void play_intro() {
    FILE* f = std::fopen(INTRO_VIDEO, "rb");
    if (!f) {
        std::printf("  \x1b[2m(no intro video found - the story is told in Ansem's voice)\x1b[0m\n\n");
        return;
    }
    std::fclose(f);
    std::printf("  \x1b[1m[Ansem]\x1b[0m \x1b[2mBefore I begin... witness what the dark shows you.\x1b[0m\n");
    std::printf("  \x1b[2m[watch intro video? y/N]\x1b[0m ");
    char buf[16];
    if (!std::fgets(buf, sizeof(buf), stdin)) return;
    if (buf[0] == 'y' || buf[0] == 'Y') {
        std::string cmd = "mpv --really-quiet --fs=no \"" + std::string(INTRO_VIDEO) + "\" 2>/dev/null";
        std::system(cmd.c_str());
    }
}

void deck_demo() {
    const std::vector<khz::Command> deck = {
        {"Slash",    "slash",    0, 12, true},
        {"Fira",     "fire",     8, 30, true},
        {"Cura",     "cure",    12,  0, true},
        {"Thundaga", "thunder", 16, 40, true},
        {"Focus",    "focus",    6,  0, true},
        {"Dark Side", "dark",   14, 35, true},
    };

    std::printf("  \x1b[1m[Ansem]\x1b[0m \x1b[2mThe command deck shifts its skin with every world.\x1b[0m\n");
    std::printf("  \x1b[2m[press enter to cycle worlds]\x1b[0m ");
    char buf[16];
    std::fgets(buf, sizeof(buf), stdin);

    const std::vector<std::string> worlds = {
        "traverse_town", "olympus", "beast_castle", "agrabah",
        "halloween_town", "hollow_bastion", "castle_oblivion",
        "twilight_town", "wnwas", "daybreak_town",
    };

    for (size_t w = 0; w < worlds.size(); ++w) {
        khz::CommandDeck deck_ui;
        deck_ui.set_world(worlds[w]);
        deck_ui.set_hp(84);
        deck_ui.set_mp(60);
        deck_ui.set_max_hp(100);
        deck_ui.set_max_mp(100);
        deck_ui.set_resonance(70 + (uint32_t)((w * 31) % 30));
        deck_ui.set_active(w % 4);
        deck_ui.set_commands(w == 4 || w == 9
            ? std::vector<khz::Command>{deck[0], deck[2], deck[5]}
            : std::vector<khz::Command>{deck[0], deck[1], deck[3], deck[4]});
        deck_ui.render();

        if (w + 1 < worlds.size()) {
            std::printf("  \x1b[2m[enter: next world]\x1b[0m ");
            std::fgets(buf, sizeof(buf), stdin);
        }
    }
}

std::vector<std::string> wrap_text(const std::string& s, size_t width) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t take = std::min(width, s.size() - pos);
        size_t cut = take;
        if (pos + take < s.size()) {
            size_t sp = s.rfind(' ', pos + take);
            if (sp != std::string::npos && sp > pos) cut = sp - pos;
        }
        out.push_back(s.substr(pos, cut));
        pos += cut;
        while (pos < s.size() && s[pos] == ' ') ++pos;
    }
    return out;
}

int act_one(khz::SaveSystem& saves) {
    khz::WorldDB db;
    if (!db.load("data/worlds/kh1.json", "data/worlds/kh2.json")) {
        std::printf("  \x1b[2m[act one] the world manifest is missing - the door stays shut\x1b[0m\n");
        return 1;
    }

    const auto& worlds = db.worlds();
    auto it = std::find_if(worlds.begin(), worlds.end(),
                           [](const khz::WorldDef& w) { return w.id == "traverse_town"; });
    if (it == worlds.end()) {
        std::printf("  \x1b[2m[act one] traverse town is lost in the dark\x1b[0m\n");
        return 1;
    }
    const khz::WorldDef& w = *it;

    std::snprintf(saves.record().world, sizeof(saves.record().world), "%s", w.id.c_str());
    saves.record().act = 1;

    std::vector<std::string> map;
    std::map<char, std::string> legend;
    {
        std::ifstream f("data/worlds/traverse_town.json");
        if (!f) {
            std::printf("  \x1b[2m[act one] the district map is missing\x1b[0m\n");
            return 1;
        }
        nlohmann::json j;
        try { f >> j; } catch (...) { return 1; }
        for (const auto& row : j.value("map", nlohmann::json::array()))
            map.push_back(row.get<std::string>());
    }
    if (map.empty()) map = {"###", "#S#", "###"};
    bool any_start = false;
    for (const auto& row : map)
        for (char c : row)
            if (c == 'S') { any_start = true; break; }
    if (!any_start) {
        std::printf("  \x1b[2m[act one] the map has no start point\x1b[0m\n");
        return 1;
    }

    int px = 1, py = 1;
    for (size_t r = 0; r < map.size(); ++r)
        for (size_t c = 0; c < map[r].size(); ++c)
            if (map[r][c] == 'S') { py = (int)r; px = (int)c; }

    bool sealed = false;
    bool visited_keyhole = false;

    auto draw = [&]() {
        std::printf("\n  \x1b[1m\x1b[38;5;141mACT ONE // %s\x1b[0m\n", w.name.c_str());
        for (size_t r = 0; r < map.size(); ++r) {
            for (size_t c = 0; c < map[r].size(); ++c) {
                if ((int)r == py && (int)c == px) {
                    std::printf("\x1b[1m@\x1b[0m");
                    continue;
                }
                char ch = map[r][c];
                if (ch == 'S') std::printf(".");
                else if (ch == 'K') std::printf("\x1b[38;5;220mK\x1b[0m");
                else if (ch == 'E') std::printf("\x1b[38;5;46mE\x1b[0m");
                else if (ch == '#') std::printf("\x1b[38;5;236m#\x1b[0m");
                else std::printf("%c", ch);
            }
            std::printf("\n");
        }
        std::printf("  \x1b[2mwasd=move | j=slash | f=fire | c=cure | t=thunder | g=guard | ?=help\x1b[0m\n");
    };

    std::printf("\n  \x1b[1m[Ansem]\x1b[0m \x1b[2mFind the keyhole in the district. Do not let the dark take your memory.\x1b[0m\n");
    draw();

    while (!sealed) {
        std::printf("  \x1b[2m[command]\x1b[0m ");
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cmd;
        for (char* p = buf; *p; ++p) {
            if (*p != '\n' && *p != '\r') cmd += (char)std::tolower((unsigned char)*p);
        }
        if (cmd == "quit" || cmd == "q") break;
        if (cmd == "help" || cmd == "?") {
            std::printf("  \x1b[2mwasd/jfctg + ? | sealed=%d keyhole=%d\x1b[0m\n", (int)sealed, (int)visited_keyhole);
            continue;
        }

        int nx = px, ny = py;
        for (char ch : cmd) {
            if (ch == 'w' || ch == 'north') ny -= 1;
            else if (ch == 's' || ch == 'south') ny += 1;
            else if (ch == 'a' || ch == 'west') nx -= 1;
            else if (ch == 'd' || ch == 'east') nx += 1;
            else if (ch == 'j') {
                std::printf("  \x1b[1mZero\x1b[0m slashes the dark air.\n");
                continue;
            }
            else if (ch == 'f') {
                std::printf("  \x1b[1mZero\x1b[0m throws fire into the dark.\n");
                continue;
            }
            else if (ch == 'c') {
                std::printf("  \x1b[1mZero\x1b[0m heals from the memory fragments.\n");
                saves.record().hp = std::min(saves.record().max_hp, saves.record().hp + 30);
                continue;
            }
            else if (ch == 't') {
                std::printf("  \x1b[1mZero\x1b[0m calls thunder across the district.\n");
                continue;
            }
            else if (ch == 'g') {
                std::printf("  \x1b[1mZero\x1b[0m raises the guard.\n");
                continue;
            }
            else continue;

            if (ny < 0 || ny >= (int)map.size() || nx < 0 || nx >= (int)map[ny].size())
                continue;
            if (map[ny][nx] != '#') {
                px = nx; py = ny;
            }
        }

        char tile = map[py][px];
        if (tile == 'K' && !visited_keyhole) {
            visited_keyhole = true;
            std::printf("\n  \x1b[38;5;220m\x1b[1mYou found the district's keyhole.\x1b[0m\n");
            // Story gate: before the flashback with Sora, Zero has no keyblade —
            // only the twin red sabres sealed the first door.
            if (saves.record().story_progress < 2)
                std::printf("  \x1b[2mZero drives the twin red sabres into the dark, and the dark closes around them - a first door, without a key.\x1b[0m\n");
            else
                std::printf("  \x1b[2mZero places the blade into the dark, and the dark closes around it.\x1b[0m\n");
            map[py][px] = '.';
            sealed = true;
        } else if (tile == 'E' && visited_keyhole) {
            sealed = true;
        } else if (tile == 'K' && visited_keyhole) {
            std::printf("  \x1b[2mThe keyhole is already sealed here.\x1b[0m\n");
        }

        if ((std::rand() % 100) < 18) {
            std::printf("\n  \x1b[38;5;196mA Heartless rises from the shadows!\x1b[0m\n");
        }
        draw();
    }

    std::printf("\n  \x1b[2m[act one] the first keyhole is marked - committing the memory\x1b[0m\n");
    if (saves.record().story_progress < 1) saves.record().story_progress = 1;
    return saves.save(khz::SaveSystem::default_path()) ? 0 : 1;
}

int act_two(khz::SaveSystem& saves, const khz::KeybladeDB& blades) {
    // Mission two: The Traverse Door. Sealing its keyhole shakes the flashback
    // with Sora loose — and the Inheritance lands: from here, keyblades answer.
    std::vector<std::string> map;
    {
        std::ifstream f("data/worlds/traverse_door.json");
        if (!f) {
            std::printf("  \x1b[2m[act two] the traverse door is lost in the dark\x1b[0m\n");
            return 1;
        }
        nlohmann::json j;
        try { f >> j; } catch (...) {
            std::printf("  \x1b[2m[act two] the door's map is unreadable\x1b[0m\n");
            return 1;
        }
        for (const auto& row : j.value("map", nlohmann::json::array()))
            map.push_back(row.get<std::string>());
    }
    if (map.empty()) map = {"#####", "#S#K#", "#.#E#", "#####"};
    bool any_start = false;
    for (const auto& row : map)
        for (char c : row)
            if (c == 'S') { any_start = true; break; }
    if (!any_start) {
        std::printf("  \x1b[2m[act two] the door has no threshold\x1b[0m\n");
        return 1;
    }

    int px = 1, py = 1;
    for (size_t r = 0; r < map.size(); ++r)
        for (size_t c = 0; c < map[r].size(); ++c)
            if (map[r][c] == 'S') { py = (int)r; px = (int)c; }

    bool sealed = false;
    bool visited_keyhole = false;

    auto draw = [&]() {
        std::printf("\n  \x1b[1m\x1b[38;5;141mACT TWO // THE TRAVERSE DOOR\x1b[0m\n");
        for (size_t r = 0; r < map.size(); ++r) {
            for (size_t c = 0; c < map[r].size(); ++c) {
                if ((int)r == py && (int)c == px) {
                    std::printf("\x1b[1m@\x1b[0m");
                    continue;
                }
                char ch = map[r][c];
                if (ch == 'S') std::printf(".");
                else if (ch == 'K') std::printf("\x1b[38;5;220mK\x1b[0m");
                else if (ch == 'E') std::printf("\x1b[38;5;81mD\x1b[0m");
                else if (ch == '#') std::printf("\x1b[38;5;236m#\x1b[0m");
                else std::printf("%c", ch);
            }
            std::printf("\n");
        }
        std::printf("  \x1b[2mwasd=move | j=slash | f=fire | c=cure | t=thunder | g=guard | ?=help\x1b[0m\n");
    };

    std::printf("\n  \x1b[1m[Ansem]\x1b[0m \x1b[2mThe door between worlds wobbles - it only does that in the presence of the liminal. Find its keyhole, Zero. And do not listen to what the door offers.\x1b[0m\n");
    std::printf("  \x1b[2mThe twin red sabres rest at your hips. No keyblade answers you yet.\x1b[0m\n");
    draw();

    while (!sealed) {
        std::printf("  \x1b[2m[command]\x1b[0m ");
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cmd;
        for (char* p = buf; *p; ++p) {
            if (*p != '\n' && *p != '\r') cmd += (char)std::tolower((unsigned char)*p);
        }
        if (cmd == "quit" || cmd == "q") break;
        if (cmd == "help" || cmd == "?") {
            std::printf("  \x1b[2msabres: j | sealed=%d keyhole=%d\x1b[0m\n", (int)sealed, (int)visited_keyhole);
            continue;
        }

        int nx = px, ny = py;
        for (char ch : cmd) {
            if (ch == 'w') ny -= 1;
            else if (ch == 's') ny += 1;
            else if (ch == 'a') nx -= 1;
            else if (ch == 'd') nx += 1;
            else if (ch == 'j') {
                std::printf("  \x1b[1mZero\x1b[0m cuts the dark air with the twin red sabres.\n");
                continue;
            }
            else if (ch == 'f') {
                std::printf("  \x1b[1mZero\x1b[0m throws fire into the dark.\n");
                continue;
            }
            else if (ch == 'c') {
                std::printf("  \x1b[1mZero\x1b[0m heals from the memory fragments.\n");
                saves.record().hp = std::min(saves.record().max_hp, saves.record().hp + 30);
                continue;
            }
            else if (ch == 't') {
                std::printf("  \x1b[1mZero\x1b[0m calls thunder across the door.\n");
                continue;
            }
            else if (ch == 'g') {
                std::printf("  \x1b[1mZero\x1b[0m raises the guard.\n");
                continue;
            }
            else continue;

            if (ny < 0 || ny >= (int)map.size() || nx < 0 || nx >= (int)map[ny].size())
                continue;
            if (map[ny][nx] != '#') {
                px = nx; py = ny;
            }
        }

        char tile = map[py][px];
        if (tile == 'K' && !visited_keyhole) {
            visited_keyhole = true;
            std::printf("\n  \x1b[38;5;220m\x1b[1mThe Door's keyhole glows — a door that leads nowhere except onward.\x1b[0m\n");
            std::printf("  \x1b[2mZero seals it with the sabres' red edge. The wobble stops — and then it does not.\x1b[0m\n");
            map[py][px] = '.';
            sealed = true;
        } else if (tile == 'E' && visited_keyhole) {
            sealed = true;
        } else if (tile == 'K' && visited_keyhole) {
            std::printf("  \x1b[2mThe keyhole is already sealed here.\x1b[0m\n");
        }
        if ((std::rand() % 100) < 18) {
            std::printf("\n  \x1b[38;5;196mA Nobody steps out of the door's shadow!\x1b[0m\n");
        }
        draw();
    }

    // Mission two complete: the flashback with Sora grants the Inheritance.
    saves.record().story_progress = 2;
    play_flashback_with_sora(saves, blades);
    return saves.save(khz::SaveSystem::default_path()) ? 0 : 1;
}

// The Inheritance by Witness: after the second door is sealed, Zero sees the
// boy on the drowned shore draw a Keyblade in the sand — and keyblades answer
// him from that moment on. Grants the Twilight Keyblade to the save.
void play_flashback_with_sora(khz::SaveSystem& saves, const khz::KeybladeDB& blades) {
    std::printf("\n  \x1b[38;5;141m\x1b[1m--- THE FLASHBACK ---\x1b[0m\n");

    std::ifstream f("data/dialogue/flashback_sora.json");
    if (f) {
        nlohmann::json j;
        try { f >> j; } catch (...) {}
        for (const auto& line : j.value("scene", nlohmann::json::array())) {
            std::printf("  %s\n", line.get<std::string>().c_str());
            std::fflush(stdout);
        }
    } else {
        std::printf("  [the drowned shore - a boy draws a key in the sand]\n");
    }

    // The grant: Twilight Keyblade answers at Zero's side.
    int32_t idx = blades.index_for_id("twilight_keyblade");
    if (idx >= 0) {
        saves.record().owned_keyblades |= (1u << (uint32_t)idx);
        saves.record().active_keyblade = (uint32_t)idx;
    }
    std::printf("\n  \x1b[38;5;220m\x1b[1mThe Twilight Keyblade answers at Zero's side.\x1b[0m\n");
    std::printf("  \x1b[2mThe twin red sabres remain at his hips - the fast, honest steel of his first act.\x1b[0m\n");
    std::printf("  \x1b[2mFrom here, the master gate is open: keyblades answer him.\x1b[0m\n");
}

// The Bazaar Between Doors: a terminal crafting flow. The stallkeeper forghes
// any blueprint the story arc allows - including the Ultima Weapon.
int moogle_flow(khz::SaveSystem& saves, khz::MoogleStall& stall,
                const khz::KeybladeDB& blades) {
    auto& rec = saves.record();
    std::printf("\n  \x1b[38;5;141m\x1b[1m--- MOOGLE STALL // %s ---\x1b[0m\n",
                stall.stallkeeper_name());
    std::printf("  \x1b[2m%s\x1b[0m\n", "A small striped tent, exactly where Zero turned around.");

    while (true) {
        auto avail = stall.available(rec);
        std::printf("\n  \x1b[2m[stall] list | 1..%zu forge | mats | munny | q\x1b[0m ",
                    avail.size());
        char buf[32];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        std::string cmd;
        for (char* p = buf; *p; ++p) {
            if (*p != '\n' && *p != '\r') cmd += (char)std::tolower((unsigned char)*p);
        }
        if (cmd == "q" || cmd == "quit" || cmd == "back") break;
        if (cmd == "list" || cmd == "l") {
            std::printf("  %-4s %-28s %-8s %-10s %s\n",
                        "#", "forge", "kind", "munny", "materials");
            for (size_t i = 0; i < avail.size(); ++i) {
                const khz::Recipe* r = avail[i];
                std::string mats;
                for (const auto& miq : r->mats) {
                    if (!mats.empty()) mats += ", ";
                    mats += std::to_string(miq.second) + " " +
                            stall.materials().name(miq.first);
                }
                std::printf("  %-4zu %-28s %-8s %6u   %s%s\n",
                            i + 1, r->name.c_str(), r->kind.c_str(), r->cost,
                            mats.c_str(),
                            stall.already_owns(*r, rec) ? "  [owned]" : "");
            }
        } else if (cmd == "mats" || cmd == "m") {
            std::printf("  [satchel] munny: %u\n", rec.munny);
            bool any = false;
            for (size_t i = 0; i < stall.materials().size(); ++i) {
                if (rec.materials[i] == 0) continue;
                any = true;
                std::printf("    %-24s x%u\n",
                            stall.materials().name((uint32_t)i).c_str(),
                            rec.materials[i]);
            }
            if (!any) std::printf("    (the satchel is empty, kupo...)\n");
        } else if (cmd == "munny") {
            std::printf("  [account] %u munny\n", rec.munny);
        } else if (!cmd.empty() && cmd[0] >= '1' && cmd[0] <= '9') {
            size_t n = (size_t)(cmd[0] - '1');
            if (n < avail.size()) {
                const khz::Recipe* r = avail[n];
                std::string flavor;
                auto res = stall.craft(*r, rec, blades, flavor);
                std::printf("  %s\n", flavor.c_str());
                if (res == khz::MoogleStall::CraftResult::Ok)
                    std::printf("  \x1b[2m[catalog] remember to save the dark's memory\x1b[0m\n");
            } else {
                std::printf("  The stall cocks an ear at that number, kupo.\n");
            }
        } else {
            std::printf("  The stallkeeper blinks. \"Kupo?\"\n");
        }
    }
    std::printf("\n  \x1b[2m%s\x1b[0m\n", "The stall is always between doors, kupo.");
    return 0;
}

int menu_loop() {
    khz::SaveSystem saves;
    saves.initialize();

    khz::KeybladeDB blades;
    blades.load("data/combat/keyblades.json");
    khz::MoogleStall stall;
    stall.load("data/crafting/material_catalog.json",
               "data/crafting/recipes.json",
               "data/crafting/moogle_stall.json");

    while (true) {
        auto& rec = saves.record();
        std::printf("\n  \x1b[1m--- DOOR TO DARKNESS ---\x1b[0m\n");
        if (rec.story_progress >= 2)
            std::printf("  \x1b[2mZero wears the %s at his side | munny %u | blades %lu/%lu\x1b[0m\n",
                        blades.by_index(rec.active_keyblade)
                            ? blades.by_index(rec.active_keyblade)->name.c_str()
                            : "Twin Red Sabres",
                        rec.munny,
                        (unsigned long)__builtin_popcount(rec.owned_keyblades),
                        blades.all().size());
        else
            std::printf("  \x1b[2mZero wears the Twin Red Sabres | munny %u | the gate is closed\x1b[0m\n",
                        rec.munny);
        std::printf("  1) begin act one - traverse town\n");
        std::printf("  %s\n", rec.story_progress >= 1
                    ? "  2) begin act two - the traverse door"
                    : "  2) begin act two - the traverse door (seal the district first)");
        std::printf("  3) visit the moogle stall\n");
        std::printf("  4) save the dark's memory\n");
        std::printf("  5) load the dark's memory\n");
        std::printf("  6) quit\n");
        std::printf("  \x1b[2m[choice]\x1b[0m ");
        char buf[16];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        switch (buf[0]) {
            case '1':
                return act_one(saves);
            case '2':
                if (saves.record().story_progress < 1) {
                    std::printf("  \x1b[2mThe second door is still sealed shut - finish the district first.\x1b[0m\n");
                    break;
                }
                return act_two(saves, blades);
            case '3':
                moogle_flow(saves, stall, blades);
                break;
            case '4':
                saves.save(khz::SaveSystem::default_path());
                break;
            case '5':
                saves.load(khz::SaveSystem::default_path());
                break;
            case '6':
                std::printf("  \x1b[2m[the dark closes the door behind you]\x1b[0m\n");
                return 0;
            default:
                break;
        }
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const bool terminal_mode = (argc > 1 && std::strcmp(argv[1], "--terminal") == 0);

    if (terminal_mode) {
        title_screen();
        play_intro();

        khz::SceneGraph graph;
        graph.add_scene(khz::prologue_scene());
        graph.play();

        deck_demo();
        return menu_loop();
    }

    // Visual game: Tron-style world selection hub + terminal adventures.
    khz::TronShell shell;
    if (!shell.init("Kingdom Hearts 0: Door to Darkness - Twilight Elysium")) {
        std::fprintf(stderr, "visual shell failed to open; try '--terminal'\n");
        return 1;
    }
    shell.run();
    return 0;
}
