#include "combat/drive_forms.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace khz {

bool DriveFormDb::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "[DriveForms] missing: %s\n", path.c_str());
        return false;
    }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[DriveForms] parse error %s: %s\n", path.c_str(), e.what());
        return false;
    }
    for (const auto& ff : j["forms"]) {
        DriveFormDef def;
        def.id = ff.value("id", "");
        def.name = ff.value("name", def.id);
        def.sigil = ff.value("sigil", "");
        def.bit = ff.value("bit", 0u);
        def.color_ansi = ff.value("color_ansi", "255");
        def.flavor = ff.value("flavor", "");
        def.str_mul = ff.value("str_mul", 100u);
        def.mag_mul = ff.value("mag_mul", 100u);
        def.def_mul = ff.value("def_mul", 100u);
        def.spd_add = ff.value("spd_add", 0);
        def.crt_add = ff.value("crt_add", 0u);
        def.mp_drain_per_turn = ff.value("mp_drain_per_turn", 0u);
        if (ff.contains("specials")) {
            for (const auto& s : ff["specials"])
                def.specials.push_back(s.get<std::string>());
        }
        def.unlock_story_progress = ff.value("unlock_story_progress", 0u);
        m_forms.push_back(std::move(def));
    }
    return true;
}

const DriveFormDef* DriveFormDb::by_bit(uint32_t bit) const {
    for (const auto& f : m_forms)
        if (f.bit == bit) return &f;
    return nullptr;
}

const DriveFormDef* DriveFormDb::by_id(const std::string& id) const {
    for (const auto& f : m_forms)
        if (f.id == id) return &f;
    return nullptr;
}

std::vector<const DriveFormDef*> DriveFormDb::available(uint32_t story_progress) const {
    std::vector<const DriveFormDef*> out;
    for (const auto& f : m_forms)
        if (story_progress >= f.unlock_story_progress) out.push_back(&f);
    return out;
}

} // namespace khz
