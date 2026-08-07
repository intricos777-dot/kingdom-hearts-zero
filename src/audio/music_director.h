#pragma once
#include "audio/audio.h"
#include <cstdint>
#include <map>
#include <string>

namespace khz {

struct WorldThemeMusic {
    std::string idle;   // AFK watchdog theme
    std::string combat; // world battle theme
    std::string boss;   // shambler boss theme (overridden per-boss)
};

// AI music director / AFK watchdog.
//
// Watches the player; when idle longer than the AFK threshold, it fades to
// the current world's idle theme. Any input snaps back to the world's
// combat theme (or the hub). Boss fights override with a boss clip. Themes
// come from data/audio/music.json.
class MusicDirector {
public:
    using ClockMs = uint64_t;

    MusicDirector() = default;

    // Attach the audio player (required before play/tick; audio may be
    // absent in headless runs, in which case the director stays silent).
    void attach(AudioPlayer& audio) { m_audio = &audio; }

    bool load(const std::string& music_json_path);

    // Called every frame with the current world, whether a boss fight is
    // active (and its clip), the host clock, and whether the player gave
    // any input this frame.
    void tick(const std::string& world_id,
              bool boss_fight,
              const std::string& boss_clip,
              ClockMs now,
              bool input_this_frame);

    // Player interacts - clear AFK immediately.
    void note_input();

    bool is_afk() const { return m_afk; }
    const std::string& current_clip() const { return m_current; }
    const WorldThemeMusic* theme_for(const std::string& world) const;

    void set_afk_threshold(ClockMs ms) { m_afk_ms = ms; }
    ClockMs afk_threshold() const { return m_afk_ms; }

    const WorldThemeMusic& hub() const { return m_hub; }

    // Immediate play regardless of state (used by the shell on world change /
    // hub return). Records the clip so subsequent ticks see no change.
    void play_now(const std::string& clip) { play_clip(clip); }

private:
    AudioPlayer* m_audio = nullptr;
    std::map<std::string, WorldThemeMusic> m_themes;
    WorldThemeMusic m_hub;
    ClockMs m_last_input = 0;
    ClockMs m_afk_ms = 45000;
    bool m_afk = false;
    bool m_in_boss = false;
    std::string m_current;

    void play_clip(const std::string& clip);
};

} // namespace khz