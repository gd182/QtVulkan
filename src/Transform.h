#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkObj {

/**
 * @brief Компонент трансформации: позиция + кватернионная ориентация.
 *        Transform component: position + quaternion orientation.
 *
 * Чистый математический класс без зависимостей от Vulkan или оконной системы.
 * Предназначен для компоновки (composition) в объектах сцены.
 *
 * Pure math class with no Vulkan or windowing dependencies.
 * Intended for composition inside scene objects.
 *
 * Ориентация хранится как единичный кватернион (glm::quat):
 *  - Yaw применяется вокруг **мировой** Y (левое умножение).
 *  - Pitch применяется вокруг **локальной** X объекта (правое умножение).
 *
 * Orientation is stored as a unit quaternion (glm::quat):
 *  - Yaw is applied around the **world** Y axis (left-multiply).
 *  - Pitch is applied around the object's **local** X axis (right-multiply).
 *
 * Такой порядок даёт arcball-поведение без дрейфа крена (roll).
 * This order gives arcball behaviour without roll drift.
 */
class Transform {
public:
    /** @brief Конструктор по умолчанию — позиция в начале координат / Default — at origin. */
    Transform() = default;

    /**
     * @brief Конструктор с начальной позицией / Constructor with initial position.
     * @param position Начальная позиция / Initial position.
     */
    explicit Transform(const glm::vec3& worldPosition);

    // Мутаторы / Mutators

    /**
     * @brief Переместить на вектор delta / Move by delta vector.
     * @param delta Смещение в мировых координатах / World-space offset.
     */
    void move(const glm::vec3& delta);

    /**
     * @brief Повернуть через кватернионы / Rotate using quaternions.
     *
     * yawQ   = angleAxis(dyaw,   worldY)  — применяется слева
     * pitchQ = angleAxis(dpitch, localX)  — применяется справа
     * orientation = normalize( yawQ * orientation * pitchQ )
     *
     * Накопленный pitch ограничивается диапазоном (-π/2 + eps, π/2 - eps).
     * Accumulated pitch is clamped to (-π/2 + eps, π/2 - eps).
     *
     * @param dyaw   Приращение yaw (рад) / Yaw increment (rad).
     * @param dpitch Приращение pitch (рад) / Pitch increment (rad).
     */
    void rotate(float dyaw, float dpitch);

    /**
     * @brief Сбросить позицию и ориентацию к началу координат.
     *        Reset position and orientation to the origin.
     */
    void reset();

    // Вычисление матриц / Matrix

    /**
     * @brief Матрица модели: T * mat4_cast(orientation).
     *        Model matrix: T * mat4_cast(orientation).
     * @return Матрица 4×4 / 4×4 matrix.
     */
    [[nodiscard]] glm::mat4 modelMatrix() const;

    // Геттеры / Accessors

    /** @brief Получить позицию / Get position. */
    [[nodiscard]] const glm::vec3& getWorldPosition()    const noexcept { return worldPosition;    }

    /** @brief Получить кватернион ориентации / Get orientation quaternion. */
    [[nodiscard]] const glm::quat& getWorldOrientation() const noexcept { return worldOrientation; }

    /** @brief Получить накопленный pitch (рад) / Get accumulated pitch (rad). */
    [[nodiscard]] float getPitchAngle() const noexcept { return pitchAngle;       }

    /** @brief Задать позицию напрямую / Set position directly. */
    void setPosition(const glm::vec3& p) { worldPosition = p; }

private:
    glm::vec3 worldPosition = {0.0f, 0.0f, 0.0f};       /** @brief Задать позицию напрямую / Set position directly. */
    glm::quat worldOrientation = {1.0f, 0.0f, 0.0f, 0.0f}; // Ориентация / Orientation
    float pitchAngle = 0.0f; // Накопленный pitch для clamping / Accumulated pitch for clamping

    // Предел pitch = π/2 − 0.01 рад / Pitch limit = π/2 − 0.01 rad
    static constexpr float kPitchLimit = 1.5607963f;
};

} // namespace vkObj
