#include "vk3dObject.h"

/**
 * @brief Конструктор с начальной позицией / Constructor with initial position.
 */
vkObj::Vk3DObject::Vk3DObject(const glm::vec3& worldPosition)
    : transformState(worldPosition)
{}

/**
 * @brief Конструктор с отдельными координатами / Constructor with individual coordinates.
 */
vkObj::Vk3DObject::Vk3DObject(float x, float y, float zoomRadius)
    : transformState(glm::vec3(x, y, zoomRadius))
{}
