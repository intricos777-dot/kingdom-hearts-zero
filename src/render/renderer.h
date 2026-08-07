#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace khz {

// ---- minimal column-major mat4 helpers (engine Vec3/Mat4 friendly) ----
struct M4 {
    float d[16] = {0};
    static M4 identity();
    static M4 perspective(float fov_y_deg, float aspect, float znear, float zfar);
    static M4 rotate_y(float rad);
    static M4 translate(float x, float y, float z);
    static M4 look_at(float ex, float ey, float ez, float cx, float cy, float cz);
    M4 operator*(const M4& o) const;
};

struct FrameInput {
    bool up = false, down = false, left = false, right = false;
    bool enter = false, esc = false;
    bool vol_up = false, vol_down = false;
    bool tab = false, back = false;
    int key = 0;            // typed ascii char, or 0
    bool quit = false;
};

// World node rendered on the Tron grid.
struct WorldNode {
    std::string id;
    std::string name;
    float x = 0, z = 0;
    float r = 0.0f, g = 0.8f, b = 1.0f; // neon color
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(int width, int height, const char* title);
    void shutdown();
    bool should_close() const { return m_should_close; }

    void begin_frame(float r, float g, float b);
    void end_frame();

    FrameInput poll_events();

    // 3D scene (Tron grid + world nodes)
    void render_grid(const std::vector<WorldNode>& nodes, float camera_yaw, size_t selected, float pulse);
    void set_perspective(float fov, float aspect, float znear, float zfar);
    void set_camera_look(float ex, float ey, float ez, float cx, float cy, float cz);

    // 2D overlay
    void draw_rect(float x, float y, float w, float h, const float* color);
    void draw_text(float x, float y, const std::string& text, bool bold, uint32_t size,
                   const float* color);
    void draw_text_center(float x, float y, const std::string& text, bool bold, uint32_t size,
                          const float* color);
    float text_width(const std::string& text, bool bold, uint32_t size);
    void draw_terminal(float x, float y, float w, float h, const std::string& title,
                       const std::vector<std::string>& lines, const float* accent,
                       const float* text_color);

    int width() const { return m_w; }
    int height() const { return m_h; }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_ctx = nullptr;
    int m_w = 0, m_h = 0;
    bool m_initialized = false;
    bool m_should_close = false;

    // GL objects
    uint32_t m_grid_prog = 0, m_world_prog = 0, m_ui_prog = 0;
    uint32_t m_grid_vao = 0, m_grid_vbo = 0;
    uint32_t m_ui_vao = 0, m_ui_vbo = 0;
    uint32_t m_line_vao = 0, m_line_vbo = 0;

    struct GlyphCache {
        uint32_t tex = 0;
        int atlas_w = 0, atlas_h = 0, cell_w = 0, cell_h = 0;
        int rows = 0, cols = 0;
        std::map<int, float> advance;
    };
    GlyphCache m_mono_small, m_mono_med, m_bold_title;

    float m_proj[16] = {0};
    float m_view[16] = {0};

    bool init_glyphs();
    bool build_cache(GlyphCache& cache, const char* font_path, uint32_t size, int cols, int rows);
    void draw_glyphs(const std::string& text, float x, float y, const GlyphCache& cache,
                     const float* color, bool center);
};

} // namespace khz
