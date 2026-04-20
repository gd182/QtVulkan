#pragma once

#include "TypesData.h"

#include <array>
#include <cstdint>
#include <vector>

namespace vkObj {

/**
 * @brief Генератор UV-сферы / UV sphere mesh generator.
 */
class Vk3dSphere {
public:
    /**
     * @brief Сгенерировать UV-меш сферы / Generate a UV sphere mesh.
     *
     * @param stacks   Число горизонтальных делений / Horizontal divisions.
     * @param slices   Число вертикальных делений / Vertical divisions.
     * @param radius   Радиус сферы / Sphere radius.
     * @param colour   RGB цвет вершин / Per-vertex colour {r,g,b}.
     * @param outV     Выходной буфер вершин / Output vertex buffer.
     * @param outI     Выходной буфер индексов / Output index buffer.
     */
    static void makeUVSphere(int stacks, int slices, float radius,
                             const std::array<float, 3>& colour,
                             std::vector<typesData::Vertex>& outV,
                             std::vector<uint32_t>& outI);
};

} // namespace vkObj
