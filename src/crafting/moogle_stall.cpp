#include "crafting/moogle_stall.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace khz {

bool MoogleStall::load(const std::string& catalog_path, const std::string& recipes_path,
                       const std::string& stall_path) {
    if (!m_mats.load(catalog_path)) return false;

    {
        std::ifstream f(stall_path);
        if (!f) {
            std::fprintf(stderr, "[Stall] missing: %s\n", stall_path.c_str());
            return false;
        }
        nlohmann::json j;
        try {
            f >> j;
        } catch (const std::exception& e) {
            std::fprintf(stderr, "[Stall] parse error %s: %s\n", stall_path.c_str(), e.what());
            return false;
        }
        m_stall_name = j.value("title", m_stall_name);
        const auto& keeper = j.value("keeper", nlohmann::json::object());
        m_greeting = keeper.value("greeting", "");
        m_departure = keeper.value("departure", "");
    }

    m_recipes.clear();
    {
        std::ifstream f(recipes_path);
        if (!f) {
            std::fprintf(stderr, "[Stall] missing recipes: %s\n", recipes_path.c_str());
            return false;
        }
        nlohmann::json j;
        try {
            f >> j;
        } catch (const std::exception& e) {
            std::fprintf(stderr, "[Stall] recipes parse error %s: %s\n", recipes_path.c_str(), e.what());
            return false;
        }
        for (const auto& r : j.value("recipes", nlohmann::json::array())) {
            Recipe rc;
            rc.id = r.value("id", "");
            rc.kind = r.value("kind", "keyblade");
            rc.name = r.value("name", rc.id);
            rc.desc = r.value("desc", "");
            rc.arc = r.value("arc", 2);
            rc.cost = r.value("cost", 0);
            bool bad_material = false;
            for (const auto& m : r.value("materials", nlohmann::json::array())) {
                int32_t mi = m_mats.index_by_id(m.value("id", ""));
                if (mi < 0) {
                    std::fprintf(stderr, "[Stall] %s: unknown material '%s'\n",
                                 rc.id.c_str(), m.value("id", "").c_str());
                    bad_material = true;
                    break;
                }
                rc.mats.emplace_back((uint8_t)mi, m.value("qty", 1));
            }
            if (bad_material) continue;
            m_recipes.push_back(std::move(rc));
        }
    }

    std::printf("\x1b[2m[Stall] %s - %zu blueprints on the shelf\x1b[0m\n",
                m_stall_name.c_str(), m_recipes.size());
    return !m_recipes.empty();
}

std::vector<const Recipe*> MoogleStall::available(const SaveRecord& rec) const {
    std::vector<const Recipe*> out;
    for (const auto& r : m_recipes)
        if (r.arc <= rec.story_progress) out.push_back(&r);
    return out;
}

bool MoogleStall::already_owns(const Recipe& r, const SaveRecord& rec) const {
    if (r.kind == "form") {
        if (r.id == "shadow_overdrive") return rec.forms_unlocked & FORM_SHADOW;
        if (r.id == "ultima_drive")     return rec.forms_unlocked & FORM_ULTIMA;
        if (r.id == "twilight_form")    return rec.forms_unlocked & FORM_TWILIGHT;
    }
    // keyblades / deck slots are resolved at craft() time (keyblade index
    // needs the KeybladeDB; deck slots never have "ownership").
    return false;
}

MoogleStall::CraftResult MoogleStall::craft(const Recipe& r, SaveRecord& rec,
                                            const KeybladeDB& db,
                                            std::string& out_flavor) {
    // story gate: the arc has not arrived — the stall refuses spiritedly.
    if (r.arc > rec.story_progress) {
        out_flavor = "The stall shakes its head, kupo. This forge is not open to you yet — the story has not caught up.";
        return CraftResult::Locked;
    }

    // ownership: refuse before spending anything.
    if (r.kind == "keyblade") {
        int32_t idx = db.index_for_id(r.id);
        if (idx < 0) {
            out_flavor = "The stall squints at the blueprint and mutters: bad copy, kupo.";
            return CraftResult::BadRecipe;
        }
        if (rec.owned_keyblades & (1u << idx)) {
            out_flavor = "You already have that one, kupo! The stall will not take your munny twice.";
            return CraftResult::AlreadyOwned;
        }
    } else if (r.kind == "form") {
        if (already_owns(r, rec)) {
            out_flavor = "That form already answers you, kupo.";
            return CraftResult::AlreadyOwned;
        }
    }

    // munny
    if (rec.munny < r.cost) {
        out_flavor = "Not enough munny, kupo! Empty pockets, empty forge.";
        return CraftResult::NoMunny;
    }

    // materials
    for (const auto& miq : r.mats) {
        if (rec.materials[miq.first] < miq.second) {
            const char* n = m_mats.name(miq.first).c_str();
            char b[160];
            std::snprintf(b, sizeof(b),
                          "Short %u %s, kupo! Come back with the dark's dues.",
                          miq.second, n);
            out_flavor = b;
            return CraftResult::NoMats;
        }
    }

    // pay
    rec.munny -= r.cost;
    for (const auto& miq : r.mats)
        rec.materials[miq.first] -= miq.second;

    // grant
    if (r.kind == "keyblade") {
        int32_t idx = db.index_for_id(r.id);
        rec.owned_keyblades |= (1u << idx);
        if (rec.active_keyblade == 0 && r.id != "twin_red_sabres")
            rec.active_keyblade = (uint32_t)idx;   // first forged blade goes to hand
        out_flavor = "\"Kupo po po po!\" The hammer falls — " + r.name +
                     " rests in Zero's hand, freshly forged.";
    } else if (r.kind == "form") {
        if (r.id == "shadow_overdrive") rec.forms_unlocked |= FORM_SHADOW;
        else if (r.id == "ultima_drive") rec.forms_unlocked |= FORM_ULTIMA;
        else if (r.id == "twilight_form") rec.forms_unlocked |= FORM_TWILIGHT;
        out_flavor = "\"Kupo!\" The stall binds the gift — " + r.name + " answered.";
    } else if (r.kind == "deck") {
        rec.deck_level += 1;
        out_flavor = "\"Kupo kupo!\" The deck is re-threaded — a command slot opens (" +
                     std::to_string(rec.deck_level) + " now, kupo).";
    } else {
        out_flavor = "The stall tilts its ears. It does not understand this blueprint, kupo.";
        return CraftResult::BadRecipe;
    }
    return CraftResult::Ok;
}

} // namespace khz