#include "save/save_system.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

namespace khz {

std::string SaveSystem::default_path() {
    const char* home = std::getenv("HOME");
    std::string base = home ? home : ".";
    return base + "/.khdoor/save.dat";
}

bool SaveSystem::initialize() {
    std::memset(&m_record, 0, sizeof(m_record));
    std::memset(&m_shadow, 0, sizeof(m_shadow));
    m_record.version = 2;
    m_record.world[0] = '\0';
    std::strncpy(m_record.world, "traverse_town", sizeof(m_record.world) - 1);
    m_record.act = 1;
    m_record.hp = 100;
    m_record.max_hp = 100;
    m_record.mp = 60;
    m_record.max_mp = 100;
    m_record.keyblade_tier = 0;
    m_record.level = 1;
    m_record.xp = 0;
    m_record.xp_to_next = 30;
    m_record.base_str = 5;
    m_record.base_mag = 5;
    m_record.base_def = 3;
    m_record.base_spd = 3;
    m_record.base_crt = 1;
    m_record.owned_keyblades = 0x1;
    m_record.active_keyblade = 0;
    m_record.forms_unlocked = 0;
    m_record.bosses_defeated = 0;
    m_shadow = m_record;
    m_corrupted = false;
    std::printf("\x1b[2m[Save] checksum save engine ready\x1b[0m\n");
    return true;
}

uint32_t SaveSystem::compute_checksum(const SaveRecord& rec) const {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&rec);
    uint32_t sum = 0xC0FFEEu;
    for (size_t i = 0; i < sizeof(rec); ++i) {
        sum = (sum + bytes[i]) * 109u + 33u;
    }
    return sum;
}

bool SaveSystem::save(const std::string& path) {
    if (path.empty()) return false;
    std::string dir = path.substr(0, path.find_last_of('/'));
    if (!dir.empty()) ::mkdir(dir.c_str(), 0700);

    m_shadow = m_record;
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    uint32_t ck = compute_checksum(m_record);
    bool ok = (std::fwrite(&m_record, sizeof(m_record), 1, f) == 1) &&
              (std::fwrite(&ck, sizeof(ck), 1, f) == 1);
    std::fclose(f);
    std::printf("\x1b[2m[Save] the dark keeps your memory\x1b[0m\n");
    return ok;
}

bool SaveSystem::self_heal() {
    std::printf("\x1b[2m[Save] the dark sorts through what you forgot...\x1b[0m\n");
    std::printf("\x1b[2m[Save]  scanning memory sectors... ok\x1b[0m\n");
    std::printf("\x1b[2m[Save]  rebuilding from the shadow copy... ok\x1b[0m\n");
    m_record = m_shadow;
    m_corrupted = false;
    return true;
}

bool SaveSystem::load(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        std::printf("\x1b[2m[Save] no record of you yet - a new story begins\x1b[0m\n");
        return false;
    }
    SaveRecord loaded{};
    uint32_t stored = 0;
    bool read_ok = (std::fread(&loaded, sizeof(loaded), 1, f) == 1) &&
                   (std::fread(&stored, sizeof(stored), 1, f) == 1);
    std::fclose(f);
    if (!read_ok) return false;

    uint32_t actual = compute_checksum(loaded);
    if (actual != stored) {
        std::printf("\x1b[38;5;196m[Save]  THE RECORD IS CORRUPTED\x1b[0m\n");
        std::printf("\x1b[38;5;196m[Save]  checksum mismatch - memory fragments\x1b[0m\n");
        m_corrupted = true;
        return self_heal();
    }
    m_record = loaded;
    m_shadow = m_record;
    std::printf("\x1b[2m[Save] the dark returns what you kept\x1b[0m\n");
    return true;
}

} // namespace khz
