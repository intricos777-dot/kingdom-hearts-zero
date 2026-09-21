#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace khz {

// A single room/area within a world.
struct AreaDef {
    std::string id;          // e.g. "traverse_district_one"
    std::string name;        // display name
    std::string world;       // parent world id
    std::string desc;        // terminal flavor
    std::vector<std::string> rows;  // ASCII map rows: S=start, .=floor, #=wall, K=keyhole, E=exit, N=npc, C=chest, D=door
    std::vector<std::string> npcs;  // NPC ids present in this area
    std::string music;       // override clip (empty = inherit from world)
};

// Expanded world manifest: each world from KH1/KH2 with its areas.
class AreaDb {
public:
    bool load(const std::string& kh1_path, const std::string& kh2_path);
    bool load_kh1(const std::string& path);
    bool load_kh2(const std::string& path);

    const std::vector<AreaDef>& areas() const { return m_areas; }

    // Areas belonging to a given world.
    std::vector<const AreaDef*> in_world(const std::string& world) const;

    // Find an area by id.
    const AreaDef* find(const std::string& id) const;

    // Count of areas in a world (for progression).
    size_t area_count(const std::string& world) const;

private:
    std::vector<AreaDef> m_areas;
    bool load_file(const std::string& path, const std::string& game_tag);
};

} // namespace khz
