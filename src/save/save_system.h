#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace khz {

// Persistent player state. Saves are checksummed; corruption self-heals.
struct SaveRecord {
    uint32_t version = 1;
    char world[32] = "traverse_town";
    uint32_t act = 1;
    uint32_t hp = 100;
    uint32_t max_hp = 100;
    uint32_t mp = 60;
    uint32_t max_mp = 100;
    uint32_t keyblade_tier = 0;   // 0 Dusk, 1 Twilight, 2 Doorwarden
    uint32_t keyholes_closed = 0;
    uint32_t memories_held = 0;
    char journal[4096] = {0};
};

class SaveSystem {
public:
    SaveSystem() = default;

    bool initialize();
    bool save(const std::string& path);
    bool load(const std::string& path);

    SaveRecord& record() { return m_record; }
    const SaveRecord& record() const { return m_record; }
    bool corrupted() const { return m_corrupted; }

    static std::string default_path();

private:
    SaveRecord m_record;
    SaveRecord m_shadow;
    bool m_corrupted = false;
    uint32_t compute_checksum(const SaveRecord& rec) const;
    bool self_heal();
};

} // namespace khz
