#pragma once
#include "render/renderer.h"
#include "render/worlds.h"
#include "audio/audio.h"
#include "audio/music_director.h"
#include "save/save_system.h"

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
    size_t m_selected = 0;
    float m_yaw = 0.0f;
    float m_pulse = 0.0f;
    uint64_t m_frame = 0;

    enum class State { SELECT, ADVENTURE, BATTLE };
    State m_state = State::SELECT;
    size_t m_beat = 0;
    size_t m_beat_count = 0;

    void draw_select(const FrameInput& in);
    void draw_adventure(const FrameInput& in);
    std::vector<std::string> deck_lines(const WorldDef& w) const;
    const float* accent_for(const WorldDef& w) const;
};

} // namespace khz
