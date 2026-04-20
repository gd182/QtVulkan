#pragma once

#include <glm/glm.hpp>

/**
 * @brief Пространство имён для типов данных, передаваемых на GPU.
 *        Namespace for data types passed to the GPU.
 */
namespace typesData {

/**
 * @brief Вершина меша / Mesh vertex.
 *
 * Содержит позицию, нормаль и цвет в object-space.
 * Contains position, normal, and colour in object space.
 */
struct Vertex {
    float pos[3]; // Позиция вершины XYZ / Vertex position XYZ
    float nor[3]; // Нормаль вершины XYZ / Vertex normal   XYZ
    float col[3]; // Цвет вершины RGB   / Vertex colour  RGB
};

/**
 * @brief Uniform Buffer Object — данные, обновляемые каждый кадр.
 *        Uniform Buffer Object — data updated every frame.
 *
 * Матрица модели перенесена в push-константы, поэтому здесь
 * только view-projection и направление света.
 *
 * Model matrix moved to push constants; only view-projection
 * and light direction remain here.
 */
struct UBO {
    glm::mat4 viewProj; // Матрица вид × проекция / View × projection matrix
    glm::vec4 lightDir; // Направление света xyz (w не используется) / Light direction xyz (w unused)
};

} // namespace typesData
