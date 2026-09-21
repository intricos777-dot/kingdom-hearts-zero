#include "crafting/materials.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace khz {

bool MaterialCatalog::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[Materials] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[Materials] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    m_mats.clear();
    for (const auto& m : j.value("materials", nlohmann::json::array())) {
        MatDef d;
        d.id = m.value("id", "");
        d.name = m.value("name", d.id);
        d.rarity = m.value("rarity", "common");
        d.desc = m.value("desc", "");
        m_mats.push_back(std::move(d));
    }
    std::printf("\x1b[2m[Materials] %zu catalogued\x1b[0m\n", m_mats.size());
    if (m_mats.size() > 32) {
        std::fprintf(stderr, "[Materials] exceeds the 32 save slots\n");
        m_mats.resize(32);
    }
    return !m_mats.empty();
}

int32_t MaterialCatalog::index_by_id(const std::string& id) const {
    for (size_t i = 0; i < m_mats.size(); ++i)
        if (m_mats[i].id == id) return (int32_t)i;
    return -1;
}

const MatDef* MaterialCatalog::by_index(uint32_t i) const {
    return i < m_mats.size() ? &m_mats[i] : nullptr;
}

} // namespace khz