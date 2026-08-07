#version 330 core
// PS2 KH1 palette: weighted flat shading with visible banding, warm fog.
in vec3 vNormal;
in vec3 vPos;
in float vFog;

uniform vec3 uBaseColor;
uniform vec3 uFogColor;
uniform vec3 uLightDir;

out vec4 FragColor;

void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    // PS2 banding: quantize the light into 4 steps.
    diff = floor(diff * 4.0) / 4.0;
    vec3 col = uBaseColor * (0.35 + 0.75 * diff);
    col = mix(col, uFogColor, vFog);
    FragColor = vec4(col, 1.0);
}
