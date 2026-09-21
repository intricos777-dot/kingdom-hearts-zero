#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "crafting/materials.h"
#include "data/keyblades.h"
#include "save/save_system.h"

namespace khz {

// What the stall forges. kind decides the grant:
//   "keyblade" -> owned_keyblades bit (by keyblades.json index)
//   "form"     -> forms_unlocked bit (shadow_overdrive/ultima_drive/twilight_form)
//   "deck"     -> deck_level += 1 (opens a command slot)
struct Recipe {
    std::string id;
    std::string kind;
    std::string name;
    std::string desc;
    uint32_t arc = 2;                 // minimum story_progress to forge
    uint32_t cost = 0;                // munny the stall demands
    std::vector<std::pair<uint8_t, uint32_t>> mats;  // (catalog index, qty)
};

// The bazaar between doors. Loads the material catalog + blueprints, checks
// affordability, gates keyblade forges behind the story, and grants the item.
class MoogleStall {
public:
    bool load(const std::string& catalog_path, const std::string& recipes_path,
              const std::string& stall_path);

    const MaterialCatalog& materials() const { return m_mats; }
    const std::vector<Recipe>& recipes() const { return m_recipes; }

    // Recipes the current story arc allows (arc <= story_progress).
    std::vector<const Recipe*> available(const SaveRecord& rec) const;

    // True if rec already owns the recipe's grant (keyblade bit / form bit).
    bool already_owns(const Recipe& r, const SaveRecord& rec) const;

    enum class CraftResult { Ok, Locked, NoMunny, NoMats, AlreadyOwned, BadRecipe };

    // Consumes munny + materials and grants the item (or refuses cleanly).
    CraftResult craft(const Recipe& r, SaveRecord& rec, const KeybladeDB& db,
                      std::string& out_flavor);

    const char* stallkeeper_name() const { return "The Stallkeeper"; }

private:
    MaterialCatalog m_mats;
    std::vector<Recipe> m_recipes;
    std::string m_stall_name = "Moogle Stall";
    std::string m_greeting;
    std::string m_departure;
};

} // namespace khz