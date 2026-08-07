#include "story/scene.h"
#include <cstdio>
#include <cstring>

namespace khz {

SceneGraph::SceneGraph() = default;

void SceneGraph::add_scene(Scene scene) {
    m_scenes.push_back(std::move(scene));
}

void SceneGraph::present_line(const std::string& text) const {
    if (m_narrator.visible) {
        std::printf("%s%s%s \xE2\x96\xB8 ", m_narrator.color.c_str(),
                    m_narrator.name.c_str(), "\x1b[0m");
    }
    std::printf("%s\n", text.c_str());
}

bool SceneGraph::play() {
    if (m_scenes.empty()) return false;
    const Scene& scene = m_scenes[m_index];

    std::printf("\n\x1b[1m\x1b[38;5;141m  %s\x1b[0m\n", scene.title.c_str());
    for (const auto& line : scene.lines) {
        present_line(line);
        std::printf("\n  \x1b[2m[press enter]\x1b[0m ");
        char buf[16];
        if (!std::fgets(buf, sizeof(buf), stdin)) break;
    }
    if (m_index + 1 < m_scenes.size()) ++m_index;
    return true;
}

Scene prologue_scene() {
    Scene s;
    s.title = "DOOR TO DARKNESS";
    s.world_id = "traverse_town";
    s.lines = {
        "Listen. And listen well, for what I am about to tell you was never",
        "written in any book of the worlds.",
        "",
        "There is a keyblade wielder the worlds do not name. They call him",
        "Zero - the unknown. He was not born of this world, nor of any world",
        "you know. He was torn from his home and cast into the dark between,",
        "wearing the coat of a Nobody, twin red blades drawn from borrowed shadow.",
        "",
        "The Heartless did not fear him, because he did not fear the dark.",
        "The heroes mistook him for an enemy, because he wore the dark's own livery.",
        "",
        "But Zero serves one master: the light - won from the shadows, never given.",
        "",
        "He has walked the edges of every story. He was the shadow behind the",
        "boy with the yellow shorts when the worlds closed their doors. He held",
        "the line in Castle Oblivion while memory frayed. He watched over the",
        "boy who wore a stolen body in the World That Never Was. He was the",
        "whisper that kept a sleeping heart from drowning, and the steady hand",
        "on a broken keyblade in a land of beasts and roses.",
        "",
        "Every heart he has touched beats inside his own - a second rhythm in",
        "the dark. He is the bridge between light and dark. The darkness cannot",
        "claim him, for the light of a hundred worlds chains his heart to theirs.",
        "The light cannot burn him, for he has made the shadows his armor.",
        "",
        "Now the Door to Darkness is opening. The heroes are scattered across",
        "a thousand worlds. And the unknown must step out of the shadows at",
        "last - close the keyholes, find the boy who lost himself to save the",
        "worlds, and seal the Door.",
        "",
        "This is the story of the one who lived between.",
    };
    return s;
}

} // namespace khz
