#include "render/renderer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#define GL_GLEXT_PROTOTYPES
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>

namespace khz {

// ---------------- M4 (column-major) ----------------
M4 M4::identity() {
    M4 m;
    m.d[0] = m.d[5] = m.d[10] = m.d[15] = 1.0f;
    return m;
}

M4 M4::perspective(float fov_y_deg, float aspect, float znear, float zfar) {
    M4 m;
    float f = 1.0f / std::tan(fov_y_deg * 3.14159265f / 360.0f);
    m.d[0] = f / aspect;
    m.d[5] = f;
    m.d[10] = (zfar + znear) / (znear - zfar);
    m.d[11] = -1.0f;
    m.d[14] = (2.0f * zfar * znear) / (znear - zfar);
    return m;
}

M4 M4::rotate_y(float rad) {
    M4 m = identity();
    float c = std::cos(rad), s = std::sin(rad);
    m.d[0] = c;  m.d[2] = s;
    m.d[8] = -s; m.d[10] = c;
    return m;
}

M4 M4::translate(float x, float y, float z) {
    M4 m = identity();
    m.d[12] = x; m.d[13] = y; m.d[14] = z;
    return m;
}

M4 M4::look_at(float ex, float ey, float ez, float cx, float cy, float cz) {
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx /= fl; fy /= fl; fz /= fl;
    float sx = fz, sy = 0.0f, sz = -fx; // up = (0,1,0)
    float sl = std::sqrt(sx*sx + sy*sy + sz*sz);
    sx /= sl; sy /= sl; sz /= sl;
    float ux = sy*fz - sz*fy;
    float uy = sz*fx - sx*fz;
    float uz = sx*fy - sy*fx;
    M4 m = identity();
    m.d[0] = sx;  m.d[4] = sy;  m.d[8]  = sz;
    m.d[1] = ux;  m.d[5] = uy;  m.d[9]  = uz;
    m.d[2] = -fx; m.d[6] = -fy; m.d[10] = -fz;
    m.d[12] = -(sx*ex + sy*ey + sz*ez);
    m.d[13] = -(ux*ex + uy*ey + uz*ez);
    m.d[14] =  (fx*ex + fy*ey + fz*ez);
    return m;
}

M4 M4::operator*(const M4& o) const {
    M4 r;
    for (int c = 0; c < 4; ++c)
        for (int rw = 0; rw < 4; ++rw) {
            float v = 0;
            for (int k = 0; k < 4; ++k) v += d[k*4+rw] * o.d[c*4+k];
            r.d[c*4+rw] = v;
        }
    return r;
}

// ---------------- Renderer ----------------
Renderer::~Renderer() { shutdown(); }

static uint32_t compile_shader(GLenum type, const char* src) {
    uint32_t sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    int ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Render] shader error: %s\n", log);
    }
    return sh;
}

static uint32_t link_program(const char* vs_path, const char* fs_path) {
    std::ifstream vf(vs_path); std::string vs((std::istreambuf_iterator<char>(vf)), {});
    std::ifstream ff(fs_path); std::string fs((std::istreambuf_iterator<char>(ff)), {});
    uint32_t v = compile_shader(GL_VERTEX_SHADER, vs.c_str());
    uint32_t f = compile_shader(GL_FRAGMENT_SHADER, fs.c_str());
    uint32_t prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Render] link error: %s\n", log);
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return prog;
}

bool Renderer::init_glyphs() {
    const char* mono = "/usr/share/fonts/TTF/DejaVuSansMono.ttf";
    const char* bold = "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf";
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "[Render] TTF_Init failed\n");
        return false;
    }
    bool a = build_cache(m_mono_small, mono, 15, 16, 8);
    bool b = build_cache(m_mono_med, mono, 19, 16, 8);
    bool c = build_cache(m_bold_title, bold, 22, 16, 8);
    return a && b && c;
}

bool Renderer::build_cache(GlyphCache& cache, const char* font_path, uint32_t size, int cols, int rows) {
    TTF_Font* font = TTF_OpenFont(font_path, (int)size);
    if (!font) {
        std::fprintf(stderr, "[Render] font load failed: %s\n", font_path);
        return false;
    }
    int cell_w = 0, cell_h = 0;
    // measure the 'W' and tallest glyph
    int minx, maxx, miny, maxy, adv;
    TTF_GlyphMetrics(font, 'W', &minx, &maxx, &miny, &maxy, &adv);
    cell_w = maxx - minx + 2;
    TTF_GlyphMetrics(font, '|', &minx, &maxx, &miny, &maxy, &adv);
    cell_h = maxy - miny + 4;

    int atlas_w = cols * cell_w, atlas_h = rows * cell_h;
    uint32_t rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
    SDL_Surface* atlas = SDL_CreateRGBSurface(0, atlas_w, atlas_h, 32, rmask, gmask, bmask, amask);
    SDL_SetSurfaceBlendMode(atlas, SDL_BLENDMODE_NONE);

    cache.cols = cols; cache.rows = rows;
    cache.cell_w = cell_w; cache.cell_h = cell_h;
    cache.atlas_w = atlas_w; cache.atlas_h = atlas_h;

    SDL_Color white = {255, 255, 255, 255};
    for (int c = 32; c <= 126; ++c) {
        SDL_Surface* g = TTF_RenderGlyph_Blended(font, (uint16_t)c, white);
        if (!g) continue;
        int ix = ((c - 32) % cols) * cell_w;
        int iy = ((c - 32) / cols) * cell_h;
        SDL_Rect dst = {ix, iy, g->w, g->h};
        SDL_BlitSurface(g, nullptr, atlas, &dst);
        SDL_FreeSurface(g);
        int a;
        TTF_GlyphMetrics(font, (uint16_t)c, &minx, &maxx, &miny, &maxy, &a);
        cache.advance[c] = (float)a;
    }
    TTF_CloseFont(font);

    glGenTextures(1, &cache.tex);
    glBindTexture(GL_TEXTURE_2D, cache.tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas_w, atlas_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(atlas);
    return true;
}

bool Renderer::init(int width, int height, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "[Render] SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::fprintf(stderr, "[Render] window: %s\n", SDL_GetError());
        return false;
    }
    m_ctx = SDL_GL_CreateContext(m_window);
    if (!m_ctx) {
        std::fprintf(stderr, "[Render] context: %s\n", SDL_GetError());
        return false;
    }
    glewExperimental = GL_TRUE;
    GLenum gerr = glewInit();
    if (gerr != GLEW_OK) {
        std::fprintf(stderr, "[Render] glew: %s\n", glewGetErrorString(gerr));
        return false;
    }
    SDL_GL_SetSwapInterval(1);
    m_w = width; m_h = height;
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    m_grid_prog = link_program("assets/shaders/tron_grid.vert", "assets/shaders/tron_grid.frag");
    m_world_prog = link_program("assets/shaders/ps2_kh2.vert", "assets/shaders/ps2_kh2.frag");
    m_ui_prog = link_program("assets/shaders/ui.vert", "assets/shaders/ui.frag");
    if (!m_grid_prog || !m_ui_prog || !m_world_prog) return false;

    // Grid floor geometry (static)
    {
        std::vector<float> verts;
        const float N = 22.0f;
        for (float i = -N; i <= N; i += 2.0f) {
            float col[3] = {0.0f, 0.85f, 0.95f};
            // line along X
            verts.insert(verts.end(), {-N, 0.0f, i, col[0], col[1], col[2]});
            verts.insert(verts.end(), { N, 0.0f, i, col[0], col[1], col[2]});
            // line along Z
            verts.insert(verts.end(), { i, 0.0f, -N, col[0], col[1], col[2]});
            verts.insert(verts.end(), { i, 0.0f,  N, col[0], col[1], col[2]});
        }
        glGenVertexArrays(1, &m_grid_vao);
        glBindVertexArray(m_grid_vao);
        glGenBuffers(1, &m_grid_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_grid_vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }

    // Dynamic line buffer (nodes)
    glGenVertexArrays(1, &m_line_vao);
    glBindVertexArray(m_line_vao);
    glGenBuffers(1, &m_line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // UI quad (dynamic)
    glGenVertexArrays(1, &m_ui_vao);
    glBindVertexArray(m_ui_vao);
    glGenBuffers(1, &m_ui_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    if (!init_glyphs()) return false;
    m_initialized = true;
    std::printf("[Render] Twilight Elysium visual shell up: %dx%d\n", width, height);
    return true;
}

void Renderer::shutdown() {
    if (!m_initialized) return;
    if (m_ctx) { SDL_GL_DeleteContext(m_ctx); m_ctx = nullptr; }
    if (m_window) { SDL_DestroyWindow(m_window); m_window = nullptr; }
    SDL_Quit();
    m_initialized = false;
}

void Renderer::begin_frame(float r, float g, float b) {
    if (!m_initialized) return;
    glViewport(0, 0, m_w, m_h);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::end_frame() {
    if (m_window) SDL_GL_SwapWindow(m_window);
}

FrameInput Renderer::poll_events() {
    FrameInput in;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) in.quit = true;
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP: in.up = true; break;
                case SDLK_DOWN: in.down = true; break;
                case SDLK_LEFT: in.left = true; break;
                case SDLK_RIGHT: in.right = true; break;
                case SDLK_RETURN: in.enter = true; break;
                case SDLK_ESCAPE: in.esc = true; break;
                case SDLK_TAB: in.tab = true; break;
                case SDLK_BACKSPACE: in.back = true; break;
                default:
                    if (e.key.keysym.sym >= 32 && e.key.keysym.sym <= 126)
                        in.key = e.key.keysym.sym;
                    break;
            }
        }
    }
    return in;
}

void Renderer::set_perspective(float fov, float aspect, float znear, float zfar) {
    M4 p = M4::perspective(fov, aspect, znear, zfar);
    std::memcpy(m_proj, p.d, sizeof(m_proj));
}

void Renderer::set_camera_look(float ex, float ey, float ez, float cx, float cy, float cz) {
    M4 v = M4::look_at(ex, ey, ez, cx, cy, cz);
    std::memcpy(m_view, v.d, sizeof(m_view));
}

void Renderer::render_grid(const std::vector<WorldNode>& nodes, float camera_yaw, size_t selected, float pulse) {
    // grid
    glUseProgram(m_grid_prog);
    glUniformMatrix4fv(glGetUniformLocation(m_grid_prog, "uProj"), 1, GL_FALSE, m_proj);
    glUniformMatrix4fv(glGetUniformLocation(m_grid_prog, "uView"), 1, GL_FALSE, m_view);
    M4 id = M4::identity();
    glUniformMatrix4fv(glGetUniformLocation(m_grid_prog, "uModel"), 1, GL_FALSE, id.d);
    glBindVertexArray(m_grid_vao);
    glDrawArrays(GL_LINES, 0, 92 * 2);

    // node markers: vertical beams + base rings
    std::vector<float> verts;
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& n = nodes[i];
        float h = (i == selected) ? 3.0f + pulse * 0.8f : 1.6f;
        float bright = (i == selected) ? 1.0f : 0.45f;
        float cr = n.r * bright, cg = n.g * bright, cb = n.b * bright;
        verts.insert(verts.end(), {n.x, 0.0f, n.z, cr, cg, cb});
        verts.insert(verts.end(), {n.x, h, n.z, cr, cg, cb});
        // base ring (cross)
        verts.insert(verts.end(), {n.x - 0.7f, 0.01f, n.z, cr, cg, cb});
        verts.insert(verts.end(), {n.x + 0.7f, 0.01f, n.z, cr, cg, cb});
        verts.insert(verts.end(), {n.x, 0.01f, n.z - 0.7f, cr, cg, cb});
        verts.insert(verts.end(), {n.x, 0.01f, n.z + 0.7f, cr, cg, cb});
        if (i == selected) {
            float pulse_a = 0.5f + 0.5f * pulse;
            verts.insert(verts.end(), {n.x - 1.6f, 0.02f, n.z, cr*pulse_a, cg*pulse_a, cb*pulse_a});
            verts.insert(verts.end(), {n.x + 1.6f, 0.02f, n.z, cr*pulse_a, cg*pulse_a, cb*pulse_a});
            verts.insert(verts.end(), {n.x, 0.02f, n.z - 1.6f, cr*pulse_a, cg*pulse_a, cb*pulse_a});
            verts.insert(verts.end(), {n.x, 0.02f, n.z + 1.6f, cr*pulse_a, cg*pulse_a, cb*pulse_a});
        }
    }
    if (!verts.empty()) {
        glBindVertexArray(m_line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, (GLsizei)(verts.size() / 6));
    }
    glBindVertexArray(0);
}

// ---------------- 2D overlay ----------------
namespace {
void quad_verts(float x, float y, float w, float h, float* out) {
    float v[24] = {
        x, y, 0, 0,  x + w, y, 1, 0,  x + w, y + h, 1, 1,
        x, y, 0, 0,  x + w, y + h, 1, 1,  x, y + h, 0, 1,
    };
    std::memcpy(out, v, sizeof(v));
}
}

void Renderer::draw_rect(float x, float y, float w, float h, const float* color) {
    float verts[24];
    quad_verts(x, y, w, h, verts);
    glUseProgram(m_ui_prog);
    glUniform4fv(glGetUniformLocation(m_ui_prog, "uColor"), 1, color);
    glUniform1i(glGetUniformLocation(m_ui_prog, "uUseTex"), 0);
    glUniform2f(glGetUniformLocation(m_ui_prog, "uViewport"), (float)m_w, (float)m_h);
    glBindVertexArray(m_ui_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

float Renderer::text_width(const std::string& text, bool bold, uint32_t size) {
    const GlyphCache& c = bold ? m_bold_title : (size <= 16 ? m_mono_small : m_mono_med);
    float w = 0;
    for (char ch : text) {
        auto it = c.advance.find((unsigned char)ch);
        w += it != c.advance.end() ? it->second : c.advance.at(' ');
    }
    return w;
}

void Renderer::draw_glyphs(const std::string& text, float x, float y, const GlyphCache& cache,
                           const float* color, bool center) {
    float total = 0;
    for (char ch : text) {
        auto it = cache.advance.find((unsigned char)ch);
        total += it != cache.advance.end() ? it->second : cache.advance.at(' ');
    }
    float pen = center ? x - total / 2.0f : x;

    glUseProgram(m_ui_prog);
    glUniform4fv(glGetUniformLocation(m_ui_prog, "uColor"), 1, color);
    glUniform1i(glGetUniformLocation(m_ui_prog, "uUseTex"), 1);
    glUniform2f(glGetUniformLocation(m_ui_prog, "uViewport"), (float)m_w, (float)m_h);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cache.tex);
    glUniform1i(glGetUniformLocation(m_ui_prog, "uTex"), 0);

    glBindVertexArray(m_ui_vao);
    for (char ch : text) {
        if ((unsigned char)ch < 32 || (unsigned char)ch > 126) ch = ' ';
        int idx = ch - 32;
        int col = idx % cache.cols, row = idx / cache.cols;
        float u0 = (col * cache.cell_w) / (float)cache.atlas_w;
        float v0 = (row * cache.cell_h) / (float)cache.atlas_h;
        float u1 = u0 + (float)cache.cell_w / cache.atlas_w;
        float v1 = v0 + (float)cache.cell_h / cache.atlas_h;
        float gw = (float)cache.cell_w, gh = (float)cache.cell_h;
        float verts[24] = {
            pen, y, u0, v0,  pen + gw, y, u1, v0,  pen + gw, y + gh, u1, v1,
            pen, y, u0, v0,  pen + gw, y + gh, u1, v1,  pen, y + gh, u0, v1,
        };
        glBindBuffer(GL_ARRAY_BUFFER, m_ui_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        auto it = cache.advance.find((unsigned char)ch);
        pen += it != cache.advance.end() ? it->second : cache.advance.at(' ');
    }
    glBindVertexArray(0);
}

void Renderer::draw_text(float x, float y, const std::string& text, bool bold, uint32_t size,
                         const float* color) {
    const GlyphCache& c = bold ? m_bold_title : (size <= 16 ? m_mono_small : m_mono_med);
    draw_glyphs(text, x, y, c, color, false);
}

void Renderer::draw_text_center(float x, float y, const std::string& text, bool bold, uint32_t size,
                                const float* color) {
    const GlyphCache& c = bold ? m_bold_title : (size <= 16 ? m_mono_small : m_mono_med);
    draw_glyphs(text, x, y, c, color, true);
}

void Renderer::draw_terminal(float x, float y, float w, float h, const std::string& title,
                             const std::vector<std::string>& lines, const float* accent,
                             const float* text_color) {
    float bg[4] = {0.02f, 0.02f, 0.04f, 0.92f};
    float border[4] = {accent[0], accent[1], accent[2], 0.9f};
    float dim_accent[4] = {accent[0] * 0.35f, accent[1] * 0.35f, accent[2] * 0.35f, 0.8f};

    draw_rect(x, y, w, h, bg);
    draw_rect(x, y, w, 2.0f, border);
    draw_rect(x, y + h - 2.0f, w, 2.0f, border);
    draw_rect(x, y, 2.0f, h, border);
    draw_rect(x + w - 2.0f, y, 2.0f, h, border);

    float pad = 12.0f;
    if (!title.empty()) {
        draw_text(x + pad, y + pad, title, true, 22, border);
        draw_rect(x + pad, y + pad + 26.0f, w - pad * 2, 1.0f, dim_accent);
    }
    float ty = y + pad + (title.empty() ? 0.0f : 40.0f);
    for (const auto& line : lines) {
        if (ty > y + h - 20.0f) break;
        draw_text(x + pad, ty, line, false, 15, text_color);
        ty += 22.0f;
    }
}

} // namespace khz
