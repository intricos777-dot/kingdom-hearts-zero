#pragma once
#include "render/renderer.h"
#include "render/worlds.h"
#include "audio/audio.h"
#include "audio/music_director.h"
#include "save/save_system.h"
#include "data/enemies.h"
#include "data/keyblades.h"
#include "combat/combat.h"

namespace khz {

// The visual game: Tron-style world selection hub + terminal adventures.
class TronShell {
public:
    bool init(const char* title);
    void run();

private:
    Renderer m_renderer;
    WorldDB m_db;
    AudioPlayer m_audio;
    MusicDirector m_music;
    SaveSystem m_saves;
    KeybladeDB m_blades;
    EnemyDB m_enemies;
    CombatEngine m_combat;
    size_t m_selected = 0;
    float m_yaw = 0.0f;
    float m_pulse = 0.0f;
    uint64_t m_frame = 0;

    enum class State { SELECT, ADVENTURE, BATTLE };
    State m_state = State::SELECT;
    size_t m_beat = 0;
    size_t m_beat_count = 0;

    // combat session state
    size_t m_battle_cursor = 0;
    std::vector<std::string> m_battle_log;

    void draw_select(const FrameInput& in);
    void draw_adventure(const FrameInput& in);
    void draw_battle(const FrameInput& in);
    void start_battle(const EnemyDef& enemy);
    void end_battle();
    std::vector<std::string> deck_lines(const WorldDef& w) const;
    const float* accent_for(const WorldDef& w) const;
};

} // namespace khz