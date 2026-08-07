#pragma once
#include <string>
#include <vector>

namespace khz {

// Ansem narrates the whole game. This is the voice that frames every scene.
struct Narrator {
    std::string name = "Ansem";
    std::string color = "\x1b[38;5;141m"; // hollow violet
    bool visible = true;
};

struct Scene {
    std::string title;
    std::vector<std::string> lines;
    std::string world_id = "traverse_town"; // deck skin active in this scene
};

class SceneGraph {
public:
    SceneGraph();

    void add_scene(Scene scene);
    bool play();                     // steps through scenes, prompting each line
    size_t index() const { return m_index; }
    void set_narrator(const Narrator& n) { m_narrator = n; }
    const Narrator& narrator() const { return m_narrator; }

private:
    std::vector<Scene> m_scenes;
    size_t m_index = 0;
    Narrator m_narrator;
    void present_line(const std::string& text) const;
};

// The Ansem prologue for "The Door Between".
Scene prologue_scene();

} // namespace khz
