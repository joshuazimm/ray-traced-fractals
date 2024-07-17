#version 430 core
out vec2 TexCoords;

void main() {
    const vec2 pos[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    TexCoords = (pos[gl_VertexID].xy + 1.0) * 0.5;
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}