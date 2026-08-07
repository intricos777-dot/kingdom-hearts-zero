#pragma once
#include <map>
#include <string>
#include <vector>

namespace khz {

// Music attribution: one entry per clip, with the creator's YouTube link.
struct MusicCredit {
    std::string clip;
    std::string creator;
    std::string title;
    std::string youtube_url;
};

// SDL_mixer-based background music player. Every clip loaded here must have a
// matching credit entry (creator + YouTube link) in assets/audio/credits.json.
// If the audio device or a clip is missing, the game runs silent - never fatal.
class AudioPlayer {
public:
    AudioPlayer() = default;
    ~AudioPlayer();

    // Opens the SDL_mixer audio device and loads credits.json from dir.
    // Returns true on success; false is non-fatal (player stays silent).
    bool init(const std::string& audio_dir);

    bool available() const { return m_ok; }

    // Preloads a clip by logical name (matches <dir>/<name>.ogg/.mp3/...).
    bool preload(const std::string& clip);
    void play(const std::string& clip, bool loop = true);
    // Plays a world's track; falls back to "hub", then silence.
    void play_world(const std::string& world_id);
    void stop();
    void set_volume(float v);   // 0..1
    float volume() const { return m_volume; }

    std::string current_clip() const { return m_current; }
    const MusicCredit* credit_for(const std::string& clip) const;
    const std::vector<MusicCredit>& credits() const { return m_credits; }

    static std::string find_file(const std::string& dir, const std::string& clip);

private:
    bool m_ok = false;
    float m_volume = 0.55f;
    std::string m_dir;
    std::string m_current;
    std::map<std::string, void*> m_clips;  // Mix_Music*
    std::vector<MusicCredit> m_credits;
    void load_credits();
};

} // namespace khz
