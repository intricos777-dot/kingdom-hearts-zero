#version 330 core
// PS2-era KH1 style: fog-drenched, soft per-vertex light.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
uniform vec3 uLightDir;

out vec3 vNormal;
out vec3 vPos;
out float vFog;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    gl_Position = uProj * uView * world;
    vNormal = mat3(uModel) * aNormal;
    vPos = world.xyz;
    vFog = clamp((length(vPos) - 8.0) / 40.0, 0.0, 1.0);
}
