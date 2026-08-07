#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

#include "story/scene.h"
#include "ui/command_deck.h"
#include "save/save_system.h"

namespace {

const char* INTRO_VIDEO = "assets/intro/intro.mp4";

void title_screen() {
    std::printf("\x1b[38;5;141m\x1b[1m");
    std::printf("  KINGDOM HEARTS: THE DOOR BETWEEN\n");
    std::printf("  =================================\n");
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

int menu_loop() {
    khz::SaveSystem saves;
    saves.initialize();

    while (true) {
        std::printf("\n  \x1b[1m--- THE DOOR BETWEEN ---\x1b[0m\n");
        std::printf("  1) begin act one - traverse town\n");
        std::printf("  2) save the dark's memory\n");
        std::printf("  3) load the dark's memory\n");
        std::printf("  4) quit\n");
        std::printf("  \x1b[2m[choice]\x1b[0m ");
        char buf[16];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
        switch (buf[0]) {
            case '1':
                std::printf("  \x1b[2m[act one not yet written - the door waits]\x1b[0m\n");
                break;
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

int main() {
    title_screen();
    play_intro();

    khz::SceneGraph graph;
    graph.add_scene(khz::prologue_scene());
    graph.play();

    deck_demo();
    return menu_loop();
}
