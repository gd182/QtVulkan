#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec3 inCol;

// Матрица модели передаётся как push-константа (отдельно для каждого объекта)
layout(push_constant) uniform PushConst {
    mat4 model;
} pc;

// Общие данные кадра: view-projection + свет
layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
    vec4 lightDir;
} ubo;

layout(location = 0) out vec3 vNorW;
layout(location = 1) out vec3 vCol;

void main() {
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    vec4 worldNor = pc.model * vec4(inNor, 0.0);
    vNorW = normalize(worldNor.xyz);
    vCol  = inCol;
    gl_Position = ubo.viewProj * worldPos;
}
