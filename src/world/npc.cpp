#include "world/npc.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace khz {

bool NPCDb::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[NPC] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[NPC] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    for (const auto& n : j["npcs"]) {
        NPCDef def;
        def.id = n.value("id", "");
        def.name = n.value("name", def.id);
        def.role = n.value("role", "world_npc");
        def.appearance = n.value("appearance", "");
        def.affiliation = n.value("affiliation", "none");

        if (n.contains("greetings")) {
            for (auto& [k, v] : n["greetings"].items())
                def.greetings[k] = v.get<std::string>();
        }
        if (n.contains("general_lines")) {
            for (const auto& l : n["general_lines"])
                def.general_lines.push_back(l.get<std::string>());
        }
        if (n.contains("schedules")) {
            for (const auto& s : n["schedules"]) {
                NPCSchedule sched;
                sched.world = s.value("world", "any");
                sched.location = s.value("location", "");
                if (s.contains("lines")) {
                    for (const auto& l : s["lines"]) {
                        ScheduledLine line;
                        line.hour = l.value("hour", 0);
                        line.text = l.value("text", "");
                        line.emotion = l.value("emotion", "neutral");
                        sched.lines[line.hour] = line;
                    }
                }
                def.schedules.push_back(sched);
            }
        }
        m_npcs.push_back(std::move(def));
    }
    return true;
}

const NPCDef* NPCDb::find(const std::string& id) const {
    for (const auto& n : m_npcs)
        if (n.id == id) return &n;
    return nullptr;
}

std::vector<const NPCDef*> NPCDb::in_world(const std::string& world) const {
    std::vector<const NPCDef*> out;
    for (const auto& n : m_npcs) {
        bool in = false;
        for (const auto& s : n.schedules) {
            if (s.world == world || s.world == "any") { in = true; break; }
        }
        if (in) out.push_back(&n);
    }
    return out;
}

std::string NPCDb::line_for(const NPCDef& npc, const std::string& world, Hour hour) const {
    for (const auto& s : npc.schedules) {
        if (s.world != world && s.world != "any") continue;
        auto it = s.lines.find(hour);
        if (it != s.lines.end()) return it->second.text;
    }
    // fallback: general line at index hour % general.size()
    if (!npc.general_lines.empty())
        return npc.general_lines[hour % npc.general_lines.size()];
    return "";
}

// ---- clock ----

NPCClock::NPCClock() = default;

std::string NPCClock::present(const NPCDb& db, const NPCDef& npc, const std::string& world) const {
    std::string line = db.line_for(npc, world, m_hour);
    if (line.empty()) return "";
    std::string emo = "neutral";
    for (const auto& s : npc.schedules) {
        if (s.world != world && s.world != "any") continue;
        auto it = s.lines.find(m_hour);
        if (it != s.lines.end()) { emo = it->second.emotion; break; }
    }
    std::string out = "[" + npc.name + ", " + emo + "] ";
    // color tags
    if (emo == "happy") out = "\x1b[38;5;220m" + out;
    else if (emo == "sad") out = "\x1b[38;5;39m" + out;
    else if (emo == "angry") out = "\x1b[38;5;196m" + out;
    else if (emo == "determined") out = "\x1b[38;5;208m" + out;
    else if (emo == "surprised") out = "\x1b[38;5;213m" + out;
    else out = "\x1b[38;5;250m" + out;
    out += line + "\x1b[0m";
    return out;
}

} // namespace khz
