#version 330 core
// PS2 KH2 palette: two-tone with a crisp rim (Nobodies' silhouettes).
in vec3 vNormal;
in vec3 vWorld;

uniform vec3 uBaseColor;
uniform vec3 uLightDir;
uniform vec3 uCamPos;

out vec4 FragColor;

void main() {
    vec3 n = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uCamPos - vWorld);
    float diff = max(dot(n, L), 0.0);
    diff = floor(diff * 3.0 + 0.5) / 3.0;   // kh2's sharper 3-step tone
    float rim = pow(1.0 - max(dot(n, V), 0.0), 3.0) * 0.8;
    vec3 col = uBaseColor * (0.28 + 0.85 * diff) + vec3(0.15, 0.35, 0.7) * rim;
    FragColor = vec4(col, 1.0);
}
