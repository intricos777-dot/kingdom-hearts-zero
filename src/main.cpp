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

namespace {

const char* INTRO_VIDEO = "assets/intro/intro.mp4";

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
    return saves.save(khz::SaveSystem::default_path()) ? 0 : 1;
}

int menu_loop() {
    khz::SaveSystem saves;
    saves.initialize();

    while (true) {
        std::printf("\n  \x1b[1m--- DOOR TO DARKNESS ---\x1b[0m\n");
        std::printf("  1) begin act one - traverse town\n");
        std::printf("  2) save the dark's memory\n");
        std::printf("  3) load the dark's memory\n");
        std::printf("  4) quit\n");
        std::printf("  \x1b[2m[choice]\x1b[0m ");
        char buf[16];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        switch (buf[0]) {
            case '1':
                return act_one(saves);
            case '2':
                saves.save(khz::SaveSystem::default_path());
                break;
            case '3':
                saves.load(khz::SaveSystem::default_path());
                break;
            case '4':
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
