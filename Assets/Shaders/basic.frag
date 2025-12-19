#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUV;

out vec4 FragColor;

uniform vec4 uColor;
uniform sampler2D uAlbedoMap;
uniform int uHasTexture;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.3, 0.8, 0.5));
    float diff = max(dot(N, L), 0.1);

    vec4 baseColor = uColor;
    if (uHasTexture > 0) {
        baseColor *= texture(uAlbedoMap, vUV);
    }

    vec3 lit = baseColor.rgb * diff;
    FragColor = vec4(lit, baseColor.a);
}

