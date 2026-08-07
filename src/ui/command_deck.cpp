#include "ui/command_deck.h"
#include "save/save_system.h"
#include <cstdio>
#include <algorithm>

namespace khz {

const std::map<std::string, WorldTheme>& world_themes() {
    static const std::map<std::string, WorldTheme> themes = {
        {"traverse_town",   {"traverse_town", "Traverse Town",
                             "Where three worlds meet, and the heart learns to walk.",
                             "\u2605", "classic", 188, 17, 74, 39, 4, 100}},
        {"olympus",         {"olympus", "Olympus Coliseum",
                             "A hero's heart is forged in the arena of gods.",
                             "\u2726", "chisel", 229, 94, 220, 208, 4, 100}},
        {"beast_castle",    {"beast_castle", "Beast's Castle",
                             "Beneath the roses beats a man, not a monster.",
                             "\u2741", "gothic", 217, 52, 196, 162, 4, 100}},
        {"agrabah",         {"agrabah", "Agrabah",
                             "A diamond in the rough glitters only in the dark.",
                             "\u2637", "etched", 228, 58, 178, 214, 4, 100}},
        {"halloween_town",  {"halloween_town", "Halloween Town",
                             "This year's fright is next year's delight.",
                             "\u2606", "stitched", 119, 22, 48, 82, 3, 100}},
        {"hollow_bastion",  {"hollow_bastion", "Hollow Bastion",
                             "The heart of the machine remembers every sin.",
                             "\u26B7", "gothic", 141, 52, 127, 196, 4, 100}},
        {"castle_oblivion", {"castle_oblivion", "Castle Oblivion",
                             "Memories are the currency of this tower.",
                             "\u25C8", "classic", 251, 60, 81, 135, 4, 100}},
        {"twilight_town",   {"twilight_town", "Twilight Town",
                             "Where day and dusk trade places without a word.",
                             "\u263E", "etched", 223, 95, 208, 172, 4, 100}},
        {"wnwas",           {"wnwas", "The World That Never Was",
                             "Nothing is nothing, and the dark dreams on.",
                             "\u2329", "hollow", 231, 232, 250, 246, 4, 100}},
        {"daybreak_town",   {"daybreak_town", "Daybreak Town",
                             "Before the war, the light still had a name.",
                             "\u2726", "chisel", 255, 24, 220, 33, 3, 100}},
    };
    return themes;
}

namespace paint {
std::string fg(uint32_t c)  { return "\x1b[38;5;" + std::to_string(c) + "m"; }
std::string bg(uint32_t c)  { return "\x1b[48;5;" + std::to_string(c) + "m"; }
std::string reset()         { return "\x1b[0m"; }
std::string bold()          { return "\x1b[1m"; }
std::string dim()           { return "\x1b[2m"; }
std::string inverse()       { return "\x1b[7m"; }
std::string hex(uint32_t r, uint32_t g, uint32_t b) {
    return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}
}

namespace {
struct FrameSet { std::string tl, tr, bl, br, h, v; };
FrameSet frame_for(const std::string& style) {
    if (style == "chisel")     return {"\xE2\x95\x94", "\xE2\x95\x97", "\xE2\x95\x9A", "\xE2\x95\x9D", "\xE2\x95\x90", "\xE2\x95\x91"};
    if (style == "etched")     return {"\xE2\x8C\x9C", "\xE2\x8C\x9D", "\xE2\x8C\x9E", "\xE2\x8C\x9F", "-", "|"};
    if (style == "stitched")   return {"+", "+", "+", "+", "-", "|"};
    if (style == "hollow")     return {"\xE2\x95\xAD", "\xE2\x95\xAE", "\xE2\x95\xB0", "\xE2\x95\xAF", "\xE2\x94\x80", "\xE2\x94\x82"};
    if (style == "gothic")     return {"\xE2\x96\x9B", "\xE2\x96\x9C", "\xE2\x96\x99", "\xE2\x96\x9A", "\xE2\x96\x88", "\xE2\x96\x90"};
    return {"\xE2\x94\x8C", "\xE2\x94\x90", "\xE2\x94\x94", "\xE2\x94\x98", "\xE2\x94\x80", "\xE2\x94\x82"};
}
}

CommandDeck::CommandDeck() {
    m_theme = world_themes().at("traverse_town");
}

void CommandDeck::set_world(const std::string& world_id) {
    auto it = world_themes().find(world_id);
    if (it != world_themes().end()) m_theme = it->second;
}

std::string CommandDeck::element_color(const std::string& element) const {
    if (element == "fire")    return paint::hex(255, 90, 40);
    if (element == "blizzard")return paint::hex(80, 200, 255);
    if (element == "thunder") return paint::hex(255, 230, 90);
    if (element == "cure")    return paint::hex(120, 255, 180);
    if (element == "focus")   return paint::hex(190, 150, 255);
    if (element == "dark")    return paint::hex(180, 90, 255);
    if (element == "light")   return paint::hex(255, 240, 190);
    return paint::hex(235, 235, 235);
}

std::string CommandDeck::frame_char(char c) const {
    (void)c;
    // frame glyphs are multi-byte; we resolve via frame_for() directly in render
    return "";
}

void CommandDeck::render_meter() const {
    const size_t w = 26;
    size_t hp = (m_max_hp ? (size_t)(m_hp * w / m_max_hp) : 0);
    size_t mp = (m_max_mp ? (size_t)(m_mp * w / m_max_mp) : 0);
    if (hp > w) hp = w;
    if (mp > w) mp = w;

    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf("  HP ");
    std::printf("%s", paint::fg(28).c_str());
    for (size_t i = 0; i < w; ++i) std::printf("%s", i < hp ? "\xE2\x96\x88" : "\xE2\x96\x91");
    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf("  %s%s%3u/%3u%s\n", paint::bold().c_str(), paint::fg(46).c_str(), m_hp, m_max_hp, paint::reset().c_str());

    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf("  MP ");
    std::printf("%s", paint::fg(27).c_str());
    for (size_t i = 0; i < w; ++i) std::printf("%s", i < mp ? "\xE2\x96\x88" : "\xE2\x96\x91");
    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf("  %s%s%3u/%3u%s\n", paint::bold().c_str(), paint::fg(51).c_str(), m_mp, m_max_mp, paint::reset().c_str());
}

std::string CommandDeck::render_command(const Command& cmd, size_t index, uint32_t width) const {
    std::string cell = " ";
    const bool active = (index == m_active);
    cell += paint::fg(active ? m_theme.bg : m_theme.fg);
    cell += paint::bg(active ? m_theme.accent : m_theme.bg);
    cell += paint::bold();
    cell += active ? "\xE2\x96\xB6 " : "   ";
    cell += cmd.name.substr(0, width);
    for (size_t i = cmd.name.size(); i < width; ++i) cell += ' ';
    cell += paint::reset();
    cell += paint::fg(m_theme.accent);
    cell += " " + std::to_string(cmd.cost) + "MP";
    cell += " ";
    cell += element_color(cmd.element);
    cell += cmd.element;
    cell += paint::reset();
    return cell;
}

void CommandDeck::render() const {
    auto frame = frame_for(m_theme.frame);
    const uint32_t inner_w = 52;

    std::printf("\n");

    // Header bar
    std::printf("%s%s", paint::fg(m_theme.accent).c_str(), paint::bold().c_str());
    std::printf("  %s %s", m_theme.sigil.c_str(), m_theme.name.c_str());
    std::printf("%s", paint::reset().c_str());
    std::printf("  %s", paint::fg(m_theme.accent2).c_str());
    std::printf("resonance [");
    uint32_t bars = m_theme.resonance / 10;
    for (uint32_t i = 0; i < 10; ++i)
        std::printf("%s", i < bars ? "\xE2\x96\x88" : "\xE2\x96\x91");
    std::printf("] %3u%%%s\n", m_theme.resonance, paint::reset().c_str());
    std::printf("  %s%s%s\n", paint::dim().c_str(), m_theme.motto.c_str(), paint::reset().c_str());

    // Top border
    std::printf("%s%s", paint::fg(m_theme.accent).c_str(), paint::bold().c_str());
    std::printf("%s", frame.tl.c_str());
    for (uint32_t i = 0; i < inner_w; ++i) std::printf("%s", frame.h.c_str());
    std::printf("%s\n", frame.tr.c_str());

    // HP / MP rows inside the frame
    std::printf("%s", paint::fg(m_theme.accent).c_str());
    std::printf("%s", frame.v.c_str());
    std::printf("%s", paint::reset().c_str());
    std::printf("  HP ");
    std::printf("%s", paint::fg(28).c_str());
    for (uint32_t i = 0; i < 34; ++i) std::printf("%s", i < (uint32_t)(m_hp * 34 / m_max_hp) ? "\xE2\x96\x88" : "\xE2\x96\x91");
    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf(" %s%3u%s%s", paint::bold().c_str(), m_hp, paint::reset().c_str(), paint::fg(m_theme.accent).c_str());
    for (size_t i = 0; i < 4; ++i) std::printf(" ");
    std::printf("%s\n", frame.v.c_str());

    std::printf("%s", paint::fg(m_theme.accent).c_str());
    std::printf("%s", frame.v.c_str());
    std::printf("%s", paint::reset().c_str());
    std::printf("  MP ");
    std::printf("%s", paint::fg(27).c_str());
    for (uint32_t i = 0; i < 34; ++i) std::printf("%s", i < (uint32_t)(m_mp * 34 / m_max_mp) ? "\xE2\x96\x88" : "\xE2\x96\x91");
    std::printf("%s", paint::fg(m_theme.fg).c_str());
    std::printf(" %s%3u%s%s", paint::bold().c_str(), m_mp, paint::reset().c_str(), paint::fg(m_theme.accent).c_str());
    for (size_t i = 0; i < 4; ++i) std::printf(" ");
    std::printf("%s\n", frame.v.c_str());

    std::printf("%s", paint::fg(m_theme.accent).c_str());
    std::printf("%s", frame.v.c_str());
    std::printf("%s", paint::reset().c_str());

    // Command cards
    std::printf("  DECK ");
    for (size_t i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];
        const bool active = (i == m_active);
        std::printf("%s", paint::fg(active ? m_theme.bg : m_theme.fg).c_str());
        std::printf("%s", paint::bg(active ? m_theme.accent : m_theme.bg).c_str());
        std::printf("%s", paint::bold().c_str());
        std::printf(" %s ", (active ? "\xE2\x96\xB6" : " "));
        std::printf("%s", cmd.name.c_str());
        while (cmd.name.size() < 12) { std::printf(" "); break; }
        std::printf("%s", paint::reset().c_str());
        std::printf("%s", element_color(cmd.element).c_str());
        std::printf(" %s%s", cmd.element.c_str(), paint::reset().c_str());
        std::printf("%s", paint::fg(active ? m_theme.bg : m_theme.accent).c_str());
        std::printf(" %uMP", cmd.cost);
        std::printf("%s", paint::reset().c_str());
        if (i + 1 < m_commands.size()) std::printf(" ");
    }
    std::printf("%s", paint::fg(m_theme.accent).c_str());
    std::printf("%s\n", frame.v.c_str());

    // Bottom border
    std::printf("%s%s", paint::fg(m_theme.accent).c_str(), paint::bold().c_str());
    std::printf("%s", frame.bl.c_str());
    for (uint32_t i = 0; i < inner_w; ++i) std::printf("%s", frame.h.c_str());
    std::printf("%s\n", frame.br.c_str());

    std::printf("%s", paint::reset().c_str());
}

std::string CommandDeck::resolve(const Command& cmd, SaveSystem& saves) {
    auto& rec = saves.record();
    std::string note;
    if (cmd.element == "slash") {
        note = cmd.name + " strikes the dark.";
    } else if (cmd.element == "fire") {
        note = cmd.name + " scorches " + std::to_string(cmd.power) + " fire damage.";
    } else if (cmd.element == "cure") {
        uint32_t healed = std::min(m_max_hp - std::min(m_hp, m_max_hp), cmd.power > 0 ? cmd.power : 50);
        m_hp = std::min(m_max_hp, m_hp + healed);
        rec.hp = m_hp;
        note = cmd.name + " heals " + std::to_string(healed) + " HP (now " + std::to_string(m_hp) + ").";
    } else {
        note = cmd.name + " resolves with no effect.";
    }
    if (rec.hp > 0 && rec.max_hp == 0) rec.max_hp = m_max_hp;
    std::printf("  [command] \x1b[1m%s\x1b[0m -> %s\n", cmd.name.c_str(), note.c_str());
    return note;
}

} // namespace khz
