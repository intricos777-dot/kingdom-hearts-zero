#include "audio/audio.h"
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <fstream>

namespace khz {

namespace {
const char* kExts[] = {".ogg", ".mp3", ".flac", ".wav", ".opus", ".m4a"};
}

AudioPlayer::~AudioPlayer() {
    for (auto& kv : m_clips) {
        if (kv.second) Mix_FreeMusic(static_cast<Mix_Music*>(kv.second));
    }
    m_clips.clear();
    if (m_ok) Mix_CloseAudio();
}

std::string AudioPlayer::find_file(const std::string& dir, const std::string& clip) {
    for (const char* ext : kExts) {
        std::string p = dir + "/" + clip + ext;
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return {};
}

bool AudioPlayer::init(const std::string& audio_dir) {
    m_dir = audio_dir;
    load_credits();

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::fprintf(stderr, "[Audio] no audio device: %s\n", Mix_GetError());
        return false;
    }
    m_ok = true;
    Mix_VolumeMusic((int)(MIX_MAX_VOLUME * m_volume));
    return true;
}

void AudioPlayer::load_credits() {
    m_credits.clear();
    std::ifstream f(m_dir + "/credits.json");
    if (!f) return;
    nlohmann::json j;
    try {
        f >> j;
    } catch (...) {
        return;
    }
    for (const auto& c : j.value("credits", nlohmann::json::array())) {
        MusicCredit mc;
        mc.clip = c.value("clip", "");
        mc.creator = c.value("creator", "");
        mc.title = c.value("title", "");
        mc.youtube_url = c.value("youtube_url", "");
        if (!mc.clip.empty()) m_credits.push_back(std::move(mc));
    }
}

const MusicCredit* AudioPlayer::credit_for(const std::string& clip) const {
    for (const auto& c : m_credits)
        if (c.clip == clip) return &c;
    return nullptr;
}

bool AudioPlayer::preload(const std::string& clip) {
    if (!m_ok) return false;
    if (m_clips.count(clip)) return true;

    std::string path = find_file(m_dir, clip);
    if (path.empty()) return false;

    Mix_Music* m = Mix_LoadMUS(path.c_str());
    if (!m) {
        std::fprintf(stderr, "[Audio] failed to load %s: %s\n", path.c_str(), Mix_GetError());
        return false;
    }
    m_clips[clip] = m;
    return true;
}

void AudioPlayer::play(const std::string& clip, bool loop) {
    if (!m_ok) return;
    if (!preload(clip)) return;
    if (m_current == clip && Mix_PlayingMusic()) return;
    Mix_HaltMusic();
    Mix_PlayMusic(static_cast<Mix_Music*>(m_clips[clip]), loop ? -1 : 1);
    m_current = clip;
}

void AudioPlayer::play_world(const std::string& world_id) {
    play(world_id);
    if (m_current.empty()) play("hub");
}

void AudioPlayer::stop() {
    if (!m_ok) return;
    Mix_HaltMusic();
    m_current.clear();
}

void AudioPlayer::set_volume(float v) {
    m_volume = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    if (m_ok) Mix_VolumeMusic((int)(MIX_MAX_VOLUME * m_volume));
}

} // namespace khz
