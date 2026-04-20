#include "Transform.h"

#include <algorithm>

// Конструкторы / Constructors

/**
 * @brief Конструктор с начальной позицией / Constructor with initial position.
 */
vkObj::Transform::Transform(const glm::vec3& worldPosition)
    : worldPosition(worldPosition)
{}

// Мутаторы / Mutators

/**
 * @brief Переместить на вектор delta / Move by delta vector.
 */
void vkObj::Transform::move(const glm::vec3& delta) {
    worldPosition += delta;
}

/**
 * @brief Повернуть через кватернионы / Rotate using quaternions.
 *
 * Порядок: yaw в мировом пространстве (левое умножение),
 *          pitch в локальном пространстве (правое умножение).
 *
 * Order: yaw in world space (left-multiply),
 *        pitch in local space (right-multiply).
 */
void vkObj::Transform::rotate(float dyaw, float dpitch) {
    // Ограничиваем суммарный pitch / Clamp accumulated pitch
    const float newPitch = pitchAngle + dpitch;
    dpitch  = std::clamp(newPitch, -kPitchLimit, kPitchLimit) - pitchAngle;
    pitchAngle += dpitch;

    // Кватернион yaw вокруг мировой оси Y / Yaw quaternion around world Y
    const glm::quat yawQ   = glm::angleAxis(dyaw,   glm::vec3(0.0f, 1.0f, 0.0f));

    // Кватернион pitch вокруг локальной оси X / Pitch quaternion around local X
    const glm::quat pitchQ = glm::angleAxis(dpitch, glm::vec3(1.0f, 0.0f, 0.0f));

    // Нормализуем после каждого шага для устранения накопленных ошибок с плавающей точкой
    // Normalise after every step to eliminate accumulated floating-point errors
    worldOrientation = glm::normalize(yawQ * worldOrientation * pitchQ);
}

/**
 * @brief Сбросить трансформацию к состоянию по умолчанию.
 *        Reset transform to the default state.
 */
void vkObj::Transform::reset() {
    worldPosition    = {0.0f, 0.0f, 0.0f};
    worldOrientation = {1.0f, 0.0f, 0.0f, 0.0f}; // единичный кватернион / identity quaternion
    pitchAngle       = 0.0f;
}

// Вычисление матриц / Matrix computation

/**
 * @brief Матрица модели: T * mat4_cast(orientation).
 *        Model matrix: T * mat4_cast(orientation).
 *
 * glm::mat4_cast переводит единичный кватернион в матрицу вращения 4×4.
 * glm::mat4_cast converts the unit quaternion to a 4×4 rotation matrix.
 */
glm::mat4 vkObj::Transform::modelMatrix() const {
    return glm::translate(glm::mat4(1.0f), worldPosition) * glm::mat4_cast(worldOrientation);
}
