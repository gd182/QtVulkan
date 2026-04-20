#pragma once

#include "Transform.h"

namespace vkObj {

/**
 * @brief Базовый класс 3D-объекта сцены.
 *        Base class for a scene 3D object.
 *
 * Компонует Transform для всей математики трансформации, не дублируя её.
 * Subclasses наследуют API перемещения, вращения и построения матрицы модели.
 *
 * Composes Transform for all transform math without duplicating it.
 * Subclasses inherit the move, rotate and model-matrix API.
 *
 * @see Transform
 */
class Vk3DObject {
public:
    /** @brief Конструктор по умолчанию / Default constructor. */
    Vk3DObject() = default;

    /**
     * @brief Конструктор с начальной позицией / Constructor with initial position.
     * @param position Начальная позиция / Initial position.
     */
    explicit Vk3DObject(const glm::vec3& worldPosition);

    /**
     * @brief Конструктор с отдельными координатами / Constructor with individual coordinates.
     * @param x X-координата / X coordinate.
     * @param y Y-координата / Y coordinate.
     * @param z Z-координата / Z coordinate.
     */
    Vk3DObject(float x, float y, float zoomRadius);

    /** @brief Виртуальный деструктор / Virtual destructor. */
    virtual ~Vk3DObject() = default;

    // Трансформация (делегирование) / Transform (delegation)

    /** @brief Переместить на delta / Move by delta. @see Transform::move */
    void move(const glm::vec3& delta) { transformState.move(delta); }

    /** @brief Повернуть через кватернионы / Rotate via quaternions. @see Transform::rotate */
    void rotate(float dyaw, float dpitch) { transformState.rotate(dyaw, dpitch); }

    /** @brief Повернуть через кватернионы / Rotate via quaternions. @see Transform::rotate */
    void resetTransform() { transformState.reset(); }

    /**
     * @brief Получить матрицу модели / Get model matrix. @see Transform::modelMatrix
     * @return Матрица модели 4×4 / 4×4 model matrix.
     */
    [[nodiscard]] glm::mat4 modelMatrix() const { return transformState.modelMatrix(); }

    // Геттеры / Accessors

    /** @brief Позиция объекта / Object position. */
    [[nodiscard]] const glm::vec3& worldPosition() const noexcept { return transformState.worldPosition();    }

    /** @brief Кватернион ориентации / Orientation quaternion. */
    [[nodiscard]] const glm::quat& worldOrientation() const noexcept { return transformState.worldOrientation(); }

    /** @brief Накопленный pitch (рад) / Accumulated pitch (rad). */
    [[nodiscard]] float pitchAngle()  const noexcept { return transformState.pitchAngle();       }

    /** @brief Задать позицию напрямую / Set position directly. */
    void setPosition(const glm::vec3& p) { transformState.setPosition(p); }

protected:
    Transform transformState; // Компонент трансформации / Transform component
};

} // namespace vkObj
