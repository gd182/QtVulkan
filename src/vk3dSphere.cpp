#include "vk3dSphere.h"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

void vkObj::Vk3dSphere::makeUVSphere(int stacks, int slices, float radius,
                                     const std::array<float, 3>& colour,
                                     std::vector<typesData::Vertex>& outV,
                                     std::vector<uint32_t>& outI)
{
    outV.clear();
    outI.clear();
    outV.reserve(static_cast<size_t>((stacks + 1) * (slices + 1)));
    outI.reserve(static_cast<size_t>(stacks * slices * 6));

    for (int i = 0; i <= stacks; ++i) {
        float v   = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = v * glm::pi<float>();

        for (int j = 0; j <= slices; ++j) {
            float u     = static_cast<float>(j) / static_cast<float>(slices);
            float theta = u * glm::two_pi<float>();

            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);

            glm::vec3 n = glm::normalize(glm::vec3(x, y, z));

            typesData::Vertex vv{};
            vv.pos[0] = n.x * radius;
            vv.pos[1] = n.y * radius;
            vv.pos[2] = n.z * radius;
            vv.nor[0] = n.x;
            vv.nor[1] = n.y;
            vv.nor[2] = n.z;
            vv.col[0] = colour[0];
            vv.col[1] = colour[1];
            vv.col[2] = colour[2];

            outV.push_back(vv);
        }
    }

    const int row = slices + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = static_cast<uint32_t>(i * row + j);
            uint32_t b = a + static_cast<uint32_t>(row);
            uint32_t c = b + 1u;
            uint32_t d = a + 1u;

            outI.push_back(a); outI.push_back(b); outI.push_back(d);
            outI.push_back(d); outI.push_back(b); outI.push_back(c);
        }
    }
}
