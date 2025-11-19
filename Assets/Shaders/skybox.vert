#version 330 core
const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);
out vec2 vUV;
void main() {
    vec2 clipPos = positions[gl_VertexID];
    vUV = clipPos * 0.5 + 0.5;
    gl_Position = vec4(clipPos, 0.0, 1.0);
}

