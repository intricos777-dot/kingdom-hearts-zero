#include "combat/door_seal.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace khz {

DoorSealLedger::DoorSealLedger(const std::vector<DoorDef>& doors) : m_doors(doors) {}

bool DoorSealLedger::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[DoorSeal] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[DoorSeal] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    for (const auto& dd : j["doors"]) {
        DoorDef d;
        d.id = dd.value("id", "");
        d.name = dd.value("name", d.id);
        d.world = dd.value("world", "");
        d.act = dd.value("act", 1u);
        d.shambler_id = dd.value("shambler_id", "");
        d.requires_shambler = dd.value("requires_shambler", true);
        d.can_keyseal = dd.value("can_keyseal", true);
        d.flavor = dd.value("flavor", "");
        m_doors.push_back(std::move(d));
    }
    return true;
}

bool DoorSealLedger::is_sealed(const std::string& door_id) const {
    return m_sealed.count(door_id) > 0;
}

const DoorDef* DoorSealLedger::find(const std::string& door_id) const {
    for (const auto& d : m_doors)
        if (d.id == door_id) return &d;
    return nullptr;
}

std::vector<const DoorDef*> DoorSealLedger::doors_in_world(const std::string& world) const {
    std::vector<const DoorDef*> out;
    for (const auto& d : m_doors)
        if (d.world == world) out.push_back(&d);
    return out;
}

std::vector<const DoorDef*> DoorSealLedger::doors_in_act(uint32_t act) const {
    std::vector<const DoorDef*> out;
    for (const auto& d : m_doors)
        if (d.act == act) out.push_back(&d);
    return out;
}

bool DoorSealLedger::seal(const std::string& door_id) {
    const DoorDef* d = find(door_id);
    if (!d) return false;
    m_sealed.insert(door_id);
    std::printf("  \x1b[38;5;220m%s sealed — %s\x1b[0m\n", d->name.c_str(), d->flavor.c_str());
    return true;
}

bool DoorSealLedger::keyseal(const std::string& door_id) {
    const DoorDef* d = find(door_id);
    if (!d || !d->can_keyseal) return false;
    if (keyseals_available == 0) {
        std::printf("  \x1b[2m(no Ultima Keyseals remaining)\x1b[0m\n");
        return false;
    }
    --keyseals_available;
    m_sealed.insert(door_id);
    std::printf("  \x1b[38;5;208m[Ultima Keyseal] %s sealed without battle. %u keyseal(s) left.\x1b[0m\n",
                d->name.c_str(), keyseals_available);
    return true;
}

bool DoorSealLedger::act_sealed(uint32_t act) const {
    for (const auto& d : m_doors)
        if (d.act == act && !is_sealed(d.id)) return false;
    return true;
}

size_t DoorSealLedger::sealed_count() const {
    return m_sealed.size();
}

size_t DoorSealLedger::total_count() const {
    return m_doors.size();
}

float DoorSealLedger::progress() const {
    return m_doors.empty() ? 0.0f : (float)m_sealed.size() / (float)m_doors.size();
}

const DoorDef* DoorSealLedger::final_door() const {
    for (const auto& d : m_doors)
        if (d.act == 5 && d.world == "wnwas") return &d;
    return nullptr;
}

} // namespace khz
