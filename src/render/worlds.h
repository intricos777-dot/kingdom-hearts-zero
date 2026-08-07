#pragma once
#include <string>
#include <vector>

namespace khz {

struct WorldDef {
    std::string id;
    std::string name;
    std::string game;      // "kh1" | "kh2"
    std::string timeline;  // when in the lore
    std::string why;       // why Zero goes there
    std::string story;     // core story beat
    std::string shader;    // ps2_kh1 | ps2_kh2
    std::vector<std::string> beats;
    float x = 0, z = 0;    // grid position in the Tron hub
    float r = 0.2f, g = 0.8f, b = 1.0f;
};

class WorldDB {
public:
    bool load(const std::string& kh1_path, const std::string& kh2_path);
    void layout_circle(float radius);
    const std::vector<WorldDef>& worlds() const { return m_worlds; }

private:
    std::vector<WorldDef> m_worlds;
    bool load_file(const std::string& path);
};

} // namespace khz
