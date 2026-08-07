#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <map>

namespace khz {

// A single command card in the deck.
struct Command {
    std::string name;      // e.g. "Slash", "Fira", "Cura"
    std::string element;   // "slash", "fire", "blizzard", "thunder", "cure", "focus"
    uint32_t cost = 0;     // MP cost
    uint32_t power = 0;
    bool unlocked = true;
};

// Every world re-skins the command deck. This is the "world-synced" UI.
struct WorldTheme {
    std::string id;         // "traverse_town"
    std::string name;       // "Traverse Town"
    std::string motto;      // flavor line drawn under the sigil
    std::string sigil;      // glyph rendered in the deck header
    std::string frame;      // box-drawing style: classic | chisel | gothic | etched | stitched | hollow
    uint32_t fg = 15;       // ANSI 256 fg
    uint32_t bg = 0;        // ANSI 256 bg
    uint32_t accent = 39;   // primary accent
    uint32_t accent2 = 33;  // secondary accent
    uint32_t slots = 4;     // deck width (commands per row)
    uint32_t resonance = 100; // world resonance percentage
};

// Theme registry: every canon world gets its own deck skin.
const std::map<std::string, WorldTheme>& world_themes();

// ANSI paint helpers.
namespace paint {
std::string fg(uint32_t c);              // "\x1b[38;5;Nm"
std::string bg(uint32_t c);              // "\x1b[48;5;Nm"
std::string reset();
std::string bold();
std::string dim();
std::string inverse();
std::string hex(uint32_t r, uint32_t g, uint32_t b); // 24-bit truecolor
}

// The world-synced command deck renderer.
class CommandDeck {
public:
    CommandDeck();

    // Select the active world's theme.
    void set_world(const std::string& world_id);

    const WorldTheme& theme() const { return m_theme; }
    const std::vector<Command>& commands() const { return m_commands; }
    size_t active() const { return m_active; }
    void set_active(size_t i) { if (i < m_commands.size()) m_active = i; }

    void set_hp(uint32_t hp) { m_hp = hp; }
    void set_max_hp(uint32_t max_hp) { m_max_hp = max_hp; }
    void set_mp(uint32_t mp) { m_mp = mp; }
    void set_max_mp(uint32_t max_mp) { m_max_mp = max_mp; }
    void set_resonance(uint32_t r) { m_theme.resonance = r; }

    void set_commands(const std::vector<Command>& cmds) { m_commands = cmds; }

    // Renders the full deck panel to stdout.
    void render() const;

    // Renders just the HP/MP meter strip.
    void render_meter() const;

private:
    WorldTheme m_theme;
    std::vector<Command> m_commands;
    uint32_t m_hp = 100;
    uint32_t m_max_hp = 100;
    uint32_t m_mp = 60;
    uint32_t m_max_mp = 100;
    size_t m_active = 0;

    std::string frame_char(char c) const;       // resolve box-drawing char by frame style
    std::string render_command(const Command& cmd, size_t index, uint32_t width) const;
    std::string element_color(const std::string& element) const;
};

} // namespace khz
