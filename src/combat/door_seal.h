#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace khz {

// A single Door to Darkness: exists inside a world, can be sealed. Sealing
// may require a Shambler to be defeated, or may be openable by the Ultima
// Keyseal bypass.
struct DoorDef {
    std::string id;          // "traverse_town_keyhole"
    std::string name;        // "The District Keyhole"
    std::string world;       // world id
    uint32_t act = 1;
    std::string shambler_id; // the boss guarding this door (empty = no boss)
    bool requires_shambler = true;
    bool can_keyseal = true; // allow Ultima Keyseal bypass
    std::string flavor;      // flavor text on seal
};

// Seal ledger: which doors are closed. Drives world-unlock logic and the
// final Door to Darkness sequence.
class DoorSealLedger {
public:
    DoorSealLedger() = default;
    explicit DoorSealLedger(const std::vector<DoorDef>& doors);

    // Load door definitions from JSON.
    bool load(const std::string& path);

    const std::vector<DoorDef>& doors() const { return m_doors; }

    // Door query.
    bool is_sealed(const std::string& door_id) const;
    const DoorDef* find(const std::string& door_id) const;
    std::vector<const DoorDef*> doors_in_world(const std::string& world) const;
    std::vector<const DoorDef*> doors_in_act(uint32_t act) const;

    // Seal a door. Returns true on success.
    bool seal(const std::string& door_id);
    // Seal with an Ultima Keyseal (consumes one). Still records it.
    bool keyseal(const std::string& door_id);

    // All doors in this act sealed?
    bool act_sealed(uint32_t act) const;

    // Progress: number of doors sealed / total.
    size_t sealed_count() const;
    size_t total_count() const;
    float progress() const;

    // The final door (Act 5, world = "wnwas").
    const DoorDef* final_door() const;

    // Ultima Keyseal count tracked by save record.
    uint32_t keyseals_available = 0;

private:
    std::vector<DoorDef> m_doors;
    std::set<std::string> m_sealed;
};

} // namespace khz
