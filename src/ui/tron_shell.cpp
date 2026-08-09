#include "ui/tron_shell.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <ctime>

namespace khz {

namespace {
const float CYAN[4] = {0.2f, 0.9f, 1.0f, 1.0f};
const float AMBER[4] = {1.0f, 0.6f, 0.2f, 1.0f};
const float VIOLET[4] = {0.6f, 0.4f, 1.0f, 1.0f};
const float WHITE[4] = {0.92f, 0.94f, 1.0f, 1.0f};
const float DIM[4] = {0.5f, 0.55f, 0.65f, 1.0f};
const float GREEN[4] = {0.4f, 1.0f, 0.7f, 1.0f};
const float RED[4] = {1.0f, 0.25f, 0.3f, 1.0f};

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
    m_blades.load("data/combat/keyblades.json");
    m_enemies.load("data/combat/enemies.json");
    m_last_act = sealed_act();
    m_combat.bind(m_saves, m_blades);
    m_audio.init("assets/audio");
    m_music.attach(m_audio);
    m_music.load("data/audio/music.json");
    m_music.play_now("hub");
    return true;
}

const float* TronShell::accent_for(const WorldDef& w) const {
    return (w.game == "kh2") ? AMBER : CYAN;
}

uint32_t TronShell::sealed_act() const {
    // highest act whose whole shambler ledger sits sealed in record().bosses_defeated
    const auto& sh = m_enemies.shamblers();
    uint32_t highest = 0;
    for (const auto& s : sh) highest = std::max(highest, s.act);
    uint32_t found = 0;
    for (uint32_t a = 1; a <= highest; ++a) {
        bool all = true;
        for (const auto& s : sh) {
            if (s.act > a) continue;
            bool sealed = false;
            for (size_t i = 0; i < sh.size(); ++i)
                if (sh[i].id == s.id && (m_saves.record().bosses_defeated & (1u << i))) { sealed = true; break; }
            if (!sealed) { all = false; break; }
        }
        if (all) found = a;
    }
    return found;
}

bool TronShell::world_locked(const WorldDef& w) const {
    // a world's door stays shut until every shambler of earlier acts is sealed
    const EnemyDef* s = m_enemies.shambler_for_world(w.id);
    if (!s) return false;
    const auto& sh = m_enemies.shamblers();
    for (size_t i = 0; i < sh.size(); ++i) {
        if (sh[i].act >= s->act) continue;
        if (!(m_saves.record().bosses_defeated & (1u << i))) return true;
    }
    return false;
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
    if (const EnemyDef* s = m_enemies.shambler_for_world(w.id)) {
        if (world_locked(w))
            out.push_back("  (shambler gate sealed)");
        else
            out.push_back("  [b] INVOKE the Shambler - act " + std::to_string(s->act));
    }
    return out;
}

void TronShell::draw_select(const FrameInput& in) {
    auto& worlds = m_db.worlds();
    if (in.left || in.up) m_selected = (m_selected + worlds.size() - 1) % worlds.size();
    if (in.right || in.down) m_selected = (m_selected + 1) % worlds.size();
    if (in.vol_down) m_audio.set_volume(m_audio.volume() - 0.1f);
    if (in.vol_up) m_audio.set_volume(m_audio.volume() + 0.1f);

    // act-unlock banner: announce when the ledger crosses into a new act
    uint32_t sealed = sealed_act();
    if (sealed > m_last_act) {
        m_act_banner = 240;
        m_last_act = sealed;
        const char* tag = "ACT TWO // KEYS AND MEMORIES";
        for (const auto& a : m_enemies.acts())
            if (a.act == sealed + 1) tag = a.title.c_str();
        std::printf("\x1b[38;5;45m[Act] the grid opens: %s\x1b[0m\n", tag);
    }
    if (m_act_banner > 0) --m_act_banner;

    if (in.enter) {
        const WorldDef& w = worlds[m_selected];
        if (world_locked(w)) {
            // sealed door: the shambler behind it outranks the sealed ledger
            const EnemyDef* s = m_enemies.shambler_for_world(w.id);
            m_gate_msg = "DOOR SEALED - " + std::to_string(s->act) +
                         " act shambler. Seal act " + std::to_string(s->act - 1) + " first.";
            return;
        }
        m_gate_msg.clear();
        m_state = State::ADVENTURE;
        m_beat = 0;
        m_beat_count = worlds[m_selected].beats.size() + 1;
        const WorldDef& w2 = worlds[m_selected];
        if (const WorldThemeMusic* t = m_music.theme_for(w2.id))
            m_music.play_now(t->combat);
        else
            m_audio.play_world(w2.id);
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
    {
        const char* act_tag = "ACT ONE - THE DOOR BETWEEN";
        for (const auto& a : m_enemies.acts())
            if (a.act == m_last_act + 1) act_tag = a.title.c_str();
        m_renderer.draw_text(22, 44,
                             std::string("TRON GRID // WORLD SELECT - [<]/[>] travel - ") + act_tag +
                                 std::string("  [sealed: act ") + std::to_string(m_last_act) + " clearest]",
                             false, 15, DIM);
    }

    // node labels above beams
    for (size_t i = 0; i < nodes.size(); ++i) {
        float sx = nodes[i].x, sz = nodes[i].z;
        // project roughly: draw label near top of beam using world->screen is complex;
        // instead draw labels in the side list panel (below) and a small floating tag via overlay
        (void)sx; (void)sz;
        if (i == m_selected) {
            std::string label = ">> " + nodes[i].name;
            if (world_locked(worlds[i])) label += "  [SEALED]";
            m_renderer.draw_text(20, 700, label, true, 22,
                                 world_locked(worlds[i]) ? RED : accent_for(worlds[i]));
        }
    }

    // Tron guide terminal (bottom panel)
    const WorldDef& w = worlds[m_selected];
    const float* accent = accent_for(w);
    std::vector<std::string> lines;
    lines.push_back("TRON:// " + w.name + (world_locked(w) ? std::string("  [DOOR SEALED]")
                                                           : std::string("  [open]")));
    lines.push_back("");
    lines.push_back("WHERE   " + w.timeline);
    lines.push_back("WHEN    " + w.why);
    lines.push_back("WHY     " + w.story);
    lines.push_back("");
    if (const EnemyDef* s = m_enemies.shambler_for_world(w.id)) {
        if (world_locked(w))
            lines.push_back("SHAMBLER act " + std::to_string(s->act) + ": " + s->name);
        else
            lines.push_back("SHAMBLER act " + std::to_string(s->act) + ": " + s->name +
                            "  [" + s->desc + "]");
    }
    if (!m_gate_msg.empty()) lines.push_back("");
    if (!m_gate_msg.empty()) lines.push_back(m_gate_msg);
    if (const MusicCredit* c = m_audio.credit_for(m_audio.current_clip())) {
        lines.push_back("MUSIC   " + c->title + " - " + c->creator);
        lines.push_back("        " + c->youtube_url);
        lines.push_back("");
    }
    lines.push_back("[enter] journey to " + w.name + "   [esc] return to the dark   [ [ ]/[ ] ] volume");
    m_renderer.draw_terminal(60, 500, 1160, 200, "TRON // GUIDE", lines, accent, WHITE);

    // act-unlock banner: a flash when the next act's doors crack open
    if (m_act_banner > 0) {
        static const float BANNER[4] = {1.0f, 0.8f, 0.2f, 1.0f};
        m_renderer.draw_text(220, 300, "THE GRID OPENED - ACT " + std::to_string(m_last_act + 1) +
                                          " // KEYS AND MEMORIES",
                             true, 30, BANNER);
        m_renderer.draw_text(220, 340, "the world-select grid re-syncs; earlier acts record clean.",
                             false, 16, WHITE);
    }
}

void TronShell::start_battle(const EnemyDef& enemy) {
    m_combat.begin(enemy);
    m_battle_cursor = 0;
    m_battle_log.clear();
    m_state = State::BATTLE;
    // boss music: the shambler's own fight track wins over the world theme
    m_music.play_now(enemy.music.empty() ? m_music.hub().combat : enemy.music);
}

void TronShell::end_battle() {
    const auto& r = m_combat.result();
    SaveRecord& rec = m_saves.record();
    if (r.victory) {
        m_combat.reward(r);
        // shambler seal ledger: bit index == json order
        const auto& sh = m_enemies.shamblers();
        for (size_t i = 0; i < sh.size(); ++i)
            if (sh[i].id == m_combat.view().enemy_id) rec.bosses_defeated |= (1u << i);
        m_saves.save(SaveSystem::default_path());
    } else if (rec.hp == 0) {
        // the dark spares Zero - revive so the journey can continue
        rec.hp = rec.max_hp;  // no permadeath in the world terminal
        m_saves.save(SaveSystem::default_path());
    }
    // return to the world with world combat theme
    const auto& w = m_db.worlds()[m_selected];
    if (const WorldThemeMusic* t = m_music.theme_for(w.id))
        m_music.play_now(t->combat);
    m_state = State::ADVENTURE;
}

void TronShell::draw_adventure(const FrameInput& in) {
    auto& worlds = m_db.worlds();
    const WorldDef& w = worlds[m_selected];
    const float* accent = accent_for(w);

    if (in.esc) { m_state = State::SELECT; m_music.play_now("hub"); return; }
    if (in.enter) {
        m_beat = std::min(m_beat + 1, m_beat_count);
        SaveRecord& rec = m_saves.record();
        std::snprintf(rec.world, sizeof(rec.world), "%s", w.id.c_str());
        rec.memories_held = std::min(m_beat, w.beats.size());
        m_saves.save(SaveSystem::default_path());
    }
    if (in.key == 'b' || in.key == 'B' || in.tab) {
        if (const EnemyDef* shambler = m_enemies.shambler_for_world(w.id)) {
            start_battle(*shambler);
            return;
        }
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
    if (shown < w.beats.size()) story.push_back("  [enter] next beat   [esc] leave " + w.name);

    // Act 2 ambience: Nobody sightings once per world while the seal-ledger works.
    static const int SIGHTING_BEAT = 3;
    const EnemyDef* sh = m_enemies.shambler_for_world(w.id);
    if (sh && sh->act >= 2 && m_beat >= SIGHTING_BEAT && m_beat < SIGHTING_BEAT + 1 &&
        !(m_nobody_seen & (1u << m_selected))) {
        m_nobody_seen |= (1u << m_selected);
        story.push_back("");
        story.push_back("  [sighting] a hooded figure watches from the gridline...");
        story.push_back("  [sighting] a Nobody. it vanishes behind a collapsing beat.");
    }

    m_renderer.draw_terminal(60, 250, 700, 430, "ANSEM // STORY", story, VIOLET, WHITE);

    // command deck terminal (world-themed)
    m_renderer.draw_terminal(780, 250, 440, 430, "COMMAND DECK", deck_lines(w), accent, WHITE);
}

void TronShell::draw_battle(const FrameInput& in) {
    const auto& v = m_combat.view();
    const auto& worlds = m_db.worlds();
    const WorldDef& w = worlds[m_selected];
    const float* accent = accent_for(w);

    auto flee = [&]() {
        end_battle();
    };

    // ---- input handling per phase ----
    if (v.phase == BattlePhase::FormSelect) {
        auto names = m_combat.form_names();
        if (in.up || in.left) m_battle_cursor = (m_battle_cursor + names.size() - 1) % names.size();
        else if (in.down || in.right) m_battle_cursor = (m_battle_cursor + 1) % names.size();
        if (in.enter || in.key == 'z') {
            m_combat.select_form(m_battle_cursor);
            m_battle_cursor = 0;
        }
        if (in.esc || in.back) flee();
    } else if (v.phase == BattlePhase::PlayerTurn) {
        if (in.up || in.left) m_battle_cursor = (m_battle_cursor + v.deck.size() - 1) % v.deck.size();
        else if (in.down || in.right) m_battle_cursor = (m_battle_cursor + 1) % v.deck.size();
        if (in.enter || in.key == 'z') {
            std::vector<std::string> lines = m_combat.act(m_battle_cursor);
            m_battle_log.insert(m_battle_log.end(), lines.begin(), lines.end());
            if (m_battle_log.size() > 40) m_battle_log.erase(m_battle_log.begin(),
                                                             m_battle_log.begin() + (m_battle_log.size() - 40));
            m_battle_cursor = 0;
        }
        if (in.esc || in.back) flee();
    } else {
        // Victory / Defeat: dismiss and apply rewards
        if (in.enter || in.esc) { end_battle(); return; }
        if (in.key == '\n' || in.key == 'z') end_battle();
    }

    // ---- render ----
    m_renderer.draw_text(20, 16, "KINGDOM HEARTS 0: DOOR TO DARKNESS", true, 22, RED);
    m_renderer.draw_text(22, 44, "BATTLE // " + w.name + " - Ansem narrates the dark", false, 15, DIM);

    // enemy panel
    std::vector<std::string> enemy_lines = {
        v.enemy_name + (v.enemy_kind == "shambler" ? "   [SHAMBLER]" : "   [HEARTLESS]"),
        "",
        "HP " + std::to_string(v.enemy_hp) + "/" + std::to_string(v.enemy_max_hp),
        "",
        "music: " + (v.music.empty() ? "(world theme)" : v.music),
    };
    m_renderer.draw_terminal(60, 100, 560, 190, "ENEMY", enemy_lines, RED, WHITE);

    // player panel
    std::vector<std::string> p = {
        "Zero",
        "",
        "HP " + std::to_string(v.hp) + "/" + std::to_string(v.max_hp),
        "MP " + std::to_string(v.mp) + "/" + std::to_string(v.max_mp),
    };
    m_renderer.draw_terminal(660, 100, 560, 190, "ZERO", p, accent, WHITE);

    // action panel: form select or deck
    if (v.phase == BattlePhase::FormSelect) {
        auto names = m_combat.form_names();
        std::vector<std::string> forms;
        for (size_t i = 0; i < names.size(); ++i)
            forms.push_back(std::string(i == m_battle_cursor ? ">> " : "    ") + names[i]);
        forms.push_back("");
        forms.push_back("select with up/down, confirm with enter");
        m_renderer.draw_terminal(120, 320, 700, 340, "CHOOSE FORM", forms, VIOLET, WHITE);
    } else if (v.phase == BattlePhase::PlayerTurn) {
        std::vector<std::string> deck;
        for (size_t i = 0; i < v.deck.size(); ++i)
            deck.push_back(std::string(i == m_battle_cursor ? ">> " : "    ") + v.deck[i].name +
                           " (" + std::to_string(v.deck[i].cost) + "MP)");
        deck.push_back("");
        deck.push_back("up/down choose   enter act   esc flee");
        m_renderer.draw_terminal(120, 320, 700, 340, "COMMAND DECK", deck, accent, WHITE);
    } else if (v.phase == BattlePhase::Victory) {
        std::vector<std::string> win = {
            "The shambler falls.",
            "",
            "XP +" + std::to_string(m_combat.result().xp),
            m_combat.result().loot_keyblade.empty()
                ? ""
                : "KEYBLADE: " + m_combat.result().loot_keyblade,
        };
        win.push_back("");
        win.push_back("press enter to continue");
        m_renderer.draw_terminal(120, 320, 700, 340, "VICTORY", win, GREEN, WHITE);
    } else if (v.phase == BattlePhase::Defeat) {
        std::vector<std::string> lose = {
            "The dark closes around Zero...",
            "",
            "memories lost: " + std::to_string(v.memories_stolen),
            "",
            "press enter to return",
        };
        m_renderer.draw_terminal(120, 320, 700, 340, "DEFEAT", lose, RED, WHITE);
    }

    // battle log
    m_renderer.draw_terminal(860, 320, 360, 340, "BATTLE LOG", m_battle_log, AMBER, WHITE);
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
        else if (m_state == State::ADVENTURE) draw_adventure(in);
        else draw_battle(in);
        m_renderer.end_frame();

        // MusicDirector / AFK watchdog: feed input + world + boss state.
        const bool any_input = in.enter || in.esc || in.left || in.right ||
                               in.up || in.down || in.vol_up || in.vol_down;
        const bool boss = (m_state == State::BATTLE);
        const std::string& boss_clip = boss ? m_combat.view().music : "";
        m_music.tick(m_state == State::SELECT ? "hub"
                                              : m_db.worlds()[m_selected].id,
                     boss, boss_clip, SDL_GetTicks64(), any_input);

        uint64_t now = SDL_GetTicks64();
        if (now - last < 16) SDL_Delay(16 - (now - last));
        last = now;
    }
}

} // namespace khz
