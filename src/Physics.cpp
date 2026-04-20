#include "Physics.h"

#include <algorithm>
#include <cmath>

namespace physics {

BallPhysics::BallPhysics()
    : ballPos(0.0f, kStartY, 0.0f)
    , ballVel(0.0f)
    , elasticity(0.7f)
{}

void BallPhysics::setElasticity(float e) {
    elasticity = std::clamp(e, 0.0f, 1.0f);
}

void BallPhysics::reset() {
    ballPos = glm::vec3(0.0f, kStartY, 0.0f);
    ballVel = glm::vec3(0.0f);
}

void BallPhysics::update(float dt) {
    // Применить гравитацию / Apply gravity
    ballVel.y += kGravity * dt;

    // Интегрировать позицию / Integrate position
    ballPos += ballVel * dt;

    // Коллизия с землёй / Ground collision
    const float contactY = kGroundY + kBallRadius;
    if (ballPos.y <= contactY) {
        ballPos.y = contactY;
        ballVel.y = -ballVel.y * elasticity;

        // Гасить крошечные отскоки / Damp tiny bounces
        if (std::abs(ballVel.y) < kMinBounceVel) {
            ballVel.y = 0.0f;
        }
    }
}

} // namespace physics
