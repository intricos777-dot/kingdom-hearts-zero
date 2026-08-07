#include "ui/tron_shell.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cmath>
#include <cstring>

namespace khz {

namespace {
const float CYAN[4] = {0.2f, 0.9f, 1.0f, 1.0f};
const float AMBER[4] = {1.0f, 0.6f, 0.2f, 1.0f};
const float VIOLET[4] = {0.6f, 0.4f, 1.0f, 1.0f};
const float WHITE[4] = {0.92f, 0.94f, 1.0f, 1.0f};
const float DIM[4] = {0.5f, 0.55f, 0.65f, 1.0f};
const float GREEN[4] = {0.4f, 1.0f, 0.7f, 1.0f};

std::vector<std::string> wrap(const std::string& s, size_t width) {
    std::vector<std::string> out;
    if (s.empty()) return out;
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
}

bool TronShell::init(const char* title) {
    if (!m_renderer.init(1280, 720, title)) return false;
    if (!m_db.load("data/worlds/kh1.json", "data/worlds/kh2.json")) return false;
    m_db.layout_circle(11.0f);
    m_beat_count = m_db.worlds().empty() ? 0 : m_db.worlds()[0].beats.size() + 1;
    m_saves.initialize();
    m_saves.load(SaveSystem::default_path());
    m_audio.init("assets/audio");
    m_audio.play("hub");
    return true;
}

const float* TronShell::accent_for(const WorldDef& w) const {
    return (w.game == "kh2") ? AMBER : CYAN;
}

std::vector<std::string> TronShell::deck_lines(const WorldDef& w) const {
    // world-themed command deck slots, fed by the persistent save state
    bool dark = (w.game == "kh2");
    const auto& rec = m_saves.record();
    std::vector<std::string> out;
    out.push_back("  " + w.name + " DECK   resonance 88%");
    out.push_back("");
    out.push_back("  HP " + std::to_string(rec.hp) + "/" + std::to_string(rec.max_hp) +
                  "   MP " + std::to_string(rec.mp) + "/" + std::to_string(rec.max_mp));
    out.push_back("");
    out.push_back("   ▶ Slash       [slash]    0MP");
    out.push_back("     " + std::string(dark ? "Firaga" : "Fira") + "        [fire]     8MP");
    out.push_back("     Cura         [cure]    12MP");
    out.push_back("     " + std::string(dark ? "Thundaga" : "Thunder") + "      [thunder]  16MP");
    if (!dark) {
        out.push_back("     Focus        [focus]     6MP");
        out.push_back("     Dark Side    [dark]     14MP");
    }
    out.push_back("");
    out.push_back("  [z] enter  [x] cast  [c] guard  [esc] return");
    return out;
}

void TronShell::draw_select(const FrameInput& in) {
    auto& worlds = m_db.worlds();
    if (in.left || in.up) m_selected = (m_selected + worlds.size() - 1) % worlds.size();
    if (in.right || in.down) m_selected = (m_selected + 1) % worlds.size();
    if (in.vol_down) m_audio.set_volume(m_audio.volume() - 0.1f);
    if (in.vol_up) m_audio.set_volume(m_audio.volume() + 0.1f);
    if (in.enter) {
        m_state = State::ADVENTURE;
        m_beat = 0;
        m_beat_count = worlds[m_selected].beats.size() + 1;
        m_audio.play_world(worlds[m_selected].id);
    }

    m_yaw += 0.008f;
    float ex = std::sin(m_yaw) * 15.0f, ey = 9.5f, ez = std::cos(m_yaw) * 15.0f;
    m_renderer.set_perspective(50.0f, (float)m_renderer.width() / (float)m_renderer.height(), 0.1f, 100.0f);
    m_renderer.set_camera_look(ex, ey, ez, 0, 1.5f, 0);

    // world nodes
    std::vector<WorldNode> nodes;
    for (const auto& w : worlds) {
        WorldNode n;
        n.id = w.id; n.name = w.name; n.x = w.x; n.z = w.z;
        n.r = w.r; n.g = w.g; n.b = w.b;
        nodes.push_back(n);
    }
    m_renderer.render_grid(nodes, m_yaw, m_selected, m_pulse);

    // title
    m_renderer.draw_text(20, 16, "KINGDOM HEARTS 0: DOOR TO DARKNESS", true, 22, CYAN);
    m_renderer.draw_text(22, 44, "TRON GRID // WORLD SELECT - select a world with [<]/[>]", false, 15, DIM);

    // node labels above beams
    for (size_t i = 0; i < nodes.size(); ++i) {
        float sx = nodes[i].x, sz = nodes[i].z;
        // project roughly: draw label near top of beam using world->screen is complex;
        // instead draw labels in the side list panel (below) and a small floating tag via overlay
        (void)sx; (void)sz;
        if (i == m_selected) {
            m_renderer.draw_text(20, 700, ">> " + nodes[i].name, true, 22, accent_for(worlds[i]));
        }
    }

    // Tron guide terminal (bottom panel)
    const WorldDef& w = worlds[m_selected];
    const float* accent = accent_for(w);
    std::vector<std::string> lines;
    lines.push_back("TRON:// " + w.name);
    lines.push_back("");
    lines.push_back("WHERE   " + w.timeline);
    lines.push_back("WHEN    " + w.why);
    lines.push_back("WHY     " + w.story);
    lines.push_back("");
    if (const MusicCredit* c = m_audio.credit_for(m_audio.current_clip())) {
        lines.push_back("MUSIC   " + c->title + " - " + c->creator);
        lines.push_back("        " + c->youtube_url);
        lines.push_back("");
    }
    lines.push_back("[enter] journey to " + w.name + "   [esc] return to the dark   [ [ ]/[ ] ] volume");
    m_renderer.draw_terminal(60, 500, 1160, 200, "TRON // GUIDE", lines, accent, WHITE);
}

void TronShell::draw_adventure(const FrameInput& in) {
    auto& worlds = m_db.worlds();
    const WorldDef& w = worlds[m_selected];
    const float* accent = accent_for(w);

    if (in.esc) { m_state = State::SELECT; m_audio.play("hub"); return; }
    if (in.enter) {
        m_beat = std::min(m_beat + 1, m_beat_count);
        SaveRecord& rec = m_saves.record();
        std::snprintf(rec.world, sizeof(rec.world), "%s", w.id.c_str());
        rec.memories_held = std::min(m_beat, w.beats.size());
        m_saves.save(SaveSystem::default_path());
    }

    m_renderer.draw_text(20, 16, "KINGDOM HEARTS 0: DOOR TO DARKNESS", true, 22, accent);
    m_renderer.draw_text(22, 44, "ANSEM NARRATES // " + w.name, false, 15, DIM);

    // header terminal: where/when/why + the music playing (with creator credit)
    std::vector<std::string> header = {
        "WORLD: " + w.name,
        "WHEN : " + w.timeline,
        "WHY  : " + w.why,
    };
    if (const MusicCredit* c = m_audio.credit_for(m_audio.current_clip())) {
        header.push_back("");
        header.push_back("TRACK: " + c->title + "  -  " + c->creator);
        header.push_back("       " + c->youtube_url);
    }
    m_renderer.draw_terminal(60, 80, 1160, 150, w.name, header, accent, WHITE);

    // story terminal: Ansem narration, advancing through beats
    std::vector<std::string> story;
    story.push_back("[Ansem]");
    story.push_back("");
    for (const auto& l : wrap(w.story, 96)) story.push_back(l);
    story.push_back("");
    size_t shown = std::min(m_beat, w.beats.size());
    for (size_t i = 0; i < shown; ++i) story.push_back("  * " + w.beats[i]);
    if (shown < w.beats.size()) story.push_back("");
    story.push_back("  [enter] next beat   [esc] leave " + w.name);
    m_renderer.draw_terminal(60, 250, 700, 430, "ANSEM // STORY", story, VIOLET, WHITE);

    // command deck terminal (world-themed)
    m_renderer.draw_terminal(780, 250, 440, 430, "COMMAND DECK", deck_lines(w), accent, WHITE);
}

void TronShell::run() {
    uint64_t last = SDL_GetTicks64();
    while (!m_renderer.should_close()) {
        FrameInput in = m_renderer.poll_events();
        if (in.quit) break;
        m_pulse = 0.5f + 0.5f * std::sin(m_frame * 0.08f);
        ++m_frame;

        m_renderer.begin_frame(0.02f, 0.03f, 0.06f);
        if (m_state == State::SELECT) draw_select(in);
        else draw_adventure(in);
        m_renderer.end_frame();

        uint64_t now = SDL_GetTicks64();
        if (now - last < 16) SDL_Delay(16 - (now - last));
        last = now;
    }
}

} // namespace khz
