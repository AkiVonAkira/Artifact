#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform vec3 uSkyTopColor;
uniform vec3 uSkyHorizonColor;
void main() {
    float t = clamp(vUV.y, 0.0, 1.0);
    vec3 color = mix(uSkyHorizonColor, uSkyTopColor, t);
    FragColor = vec4(color, 1.0);
}

