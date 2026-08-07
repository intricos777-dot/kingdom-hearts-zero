#include "audio/music_director.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <fstream>

namespace khz {

bool MusicDirector::load(const std::string& music_json_path) {
    std::ifstream f(music_json_path);
    if (!f) {
        std::fprintf(stderr, "[MusicDirector] missing: %s\n", music_json_path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[MusicDirector] parse error %s: %s\n",
                     music_json_path.c_str(), e.what());
        return false;
    }

    const auto hub = j.value("hub", nlohmann::json::object());
    m_hub.idle = hub.value("idle", std::string("hub"));
    m_hub.combat = hub.value("combat", std::string("hub"));
    m_hub.boss = m_hub.combat;

    m_themes.clear();
    for (const auto& [world, t] : j.value("worlds", nlohmann::json::object()).items()) {
        WorldThemeMusic m;
        m.idle = t.value("idle", m_hub.idle);
        m.combat = t.value("combat", m_hub.combat);
        m.boss = t.value("boss", m.combat);
        m_themes[world] = std::move(m);
    }
    std::printf("\x1b[2m[MusicDirector] %zu world themes mapped\x1b[0m\n", m_themes.size());
    return true;
}

void MusicDirector::play_clip(const std::string& clip) {
    if (!m_audio || !m_audio->available() || clip.empty() || clip == m_current) return;
    m_audio->play(clip, true);
    m_current = clip;
}

void MusicDirector::note_input() {
    m_afk = false;
}

const WorldThemeMusic* MusicDirector::theme_for(const std::string& world) const {
    auto it = m_themes.find(world);
    return it == m_themes.end() ? nullptr : &it->second;
}

void MusicDirector::tick(const std::string& world_id,
                         bool boss_fight,
                         const std::string& boss_clip,
                         ClockMs now,
                         bool input_this_frame) {
    if (input_this_frame) {
        m_last_input = now;
        m_afk = false;
    }
    if (m_last_input == 0) m_last_input = now;
    const bool boss_changed = (boss_fight != m_in_boss);
    m_in_boss = boss_fight;

    WorldThemeMusic theme = m_hub;
    if (const WorldThemeMusic* t = theme_for(world_id)) theme = *t;

    // AFK watchdog: idle time only counts outside boss fights.
    if (!boss_fight) {
        if (!m_afk && (now - m_last_input) >= m_afk_ms) {
            m_afk = true;
            std::printf("\x1b[2m[Watchdog] Zero... the world still plays for you. (AFK)\x1b[0m\n");
        }
    } else {
        m_afk = false;
    }

    std::string want;
    if (boss_fight) {
        want = boss_clip.empty() ? theme.boss : boss_clip;  // boss clip wins
    } else if (m_afk) {
        want = theme.idle;                                   // idle theme
    } else {
        want = theme.combat;                                 // world battle theme
    }

    if (boss_changed || want != m_current) play_clip(want);
}

} // namespace khz