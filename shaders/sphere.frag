#version 450

layout(location = 0) in vec3 vNorW;
layout(location = 1) in vec3 vCol;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
    vec4 lightDir;
} ubo;

void main() {
    vec3 n = normalize(vNorW);
    vec3 l = normalize(ubo.lightDir.xyz);

    float ndotl = max(dot(n, l), 0.0);
    // Диффузное освещение + вершинный цвет / Diffuse lighting with per-vertex colour
    vec3 color = vCol * (0.15 + 0.85 * ndotl);

    outColor = vec4(color, 1.0);
}
