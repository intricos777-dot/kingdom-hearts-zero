#version 330 core
in vec2 vUV;

uniform vec4 uColor;
uniform int uUseTex;
uniform sampler2D uTex;

out vec4 FragColor;

void main() {
    vec4 col = uColor;
    if (uUseTex == 1) {
        col = texture(uTex, vUV) * uColor;
    }
    FragColor = col;
}
