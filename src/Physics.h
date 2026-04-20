#pragma once

#include <glm/glm.hpp>

namespace physics {

/**
 * @brief Простая физика шара (падение + отскок от плоскости).
 *        Simple ball physics (falling + bouncing off a plane).
 */
class BallPhysics {
public:
    BallPhysics();

    // Обновление / Update
    void update(float dt);
    void reset();

    // Параметры / Parameters
    void  setElasticity(float e);
    float getElasticity() const { return elasticity; }

    // Состояние / State
    glm::vec3 getBallPosition() const { return ballPos; }

    // Константы сцены / Scene constants
    static constexpr float kBallRadius = 0.5f; // Визуальный радиус шара (model scale)
    static constexpr float kGroundY    = -1.5f; // Y-координата земли

private:
    glm::vec3 ballPos;         // Позиция центра шара
    glm::vec3 ballVel;         // Скорость шара

    float elasticity;          // Коэффициент упругости [0..1]

    static constexpr float kGravity = -9.8f;       // Ускорение свободного падения
    static constexpr float kStartY = 2.5f;         // Начальная высота центра шара
    static constexpr float kMinBounceVel = 0.08f;  // Порог гашения малых отскоков
};

} // namespace physics
