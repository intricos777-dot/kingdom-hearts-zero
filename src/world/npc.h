#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace khz {

// Time of day in a world (0-23 hours).
using Hour = uint32_t;

// A scheduled line of dialogue an NPC says at a given time in a given world.
struct ScheduledLine {
    Hour hour = 0;           // 0..23
    std::string text;
    std::string emotion;     // "neutral", "happy", "sad", "angry", "surprised", "determined"
};

// An NPC's full schedule: where they are and what they say, keyed by world.
struct NPCSchedule {
    std::string world;       // world id, or "any" for always-available
    std::map<Hour, ScheduledLine> lines;  // hour -> line
    std::string location;    // e.g. "district_one", "clock_tower"
};

// A named NPC character.
struct NPCDef {
    std::string id;
    std::string name;
    std::string role;        // "party", "ally", "merchant", "guide", "boss", "world_npc"
    std::string appearance;  // short visual description
    std::string affiliation; // "none", "destiny_islands", "organization", "disney", "self"
    std::vector<NPCSchedule> schedules;
    std::map<std::string, std::string> greetings;  // by world id
    std::vector<std::string> general_lines;        // catch-all chatter
};

// Loads and serves NPC data from a JSON manifest.
class NPCDb {
public:
    bool load(const std::string& path);

    const std::vector<NPCDef>& npcs() const { return m_npcs; }

    // Find an NPC by id.
    const NPCDef* find(const std::string& id) const;

    // NPCs available in a given world.
    std::vector<const NPCDef*> in_world(const std::string& world) const;

    // Get the line an NPC says at the given hour in the given world.
    // Returns "" if the NPC has nothing to say at this time/world.
    std::string line_for(const NPCDef& npc, const std::string& world, Hour hour) const;

private:
    std::vector<NPCDef> m_npcs;
};

// Lightweight runtime that the terminal host drives: tracks the clock and
// queries the NPCDb for the line to show.
class NPCClock {
public:
    NPCClock();

    void set_hour(Hour h) { m_hour = h % 24; }
    Hour hour() const { return m_hour; }
    void advance() { m_hour = (m_hour + 1) % 24; }
    void advance(Hour n) { m_hour = (m_hour + n) % 24; }

    // Returns "[name, emotion] text" ready for terminal output.
    std::string present(const NPCDb& db, const NPCDef& npc, const std::string& world) const;

private:
    Hour m_hour = 12;
};

} // namespace khz
