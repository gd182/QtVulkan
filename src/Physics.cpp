#include "Physics.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace physics {

// ─── Public ───────────────────────────────────────────────────────────────

void PhysicsWorld::update(float dt)
{
    for (auto& b : bodies)
        integrateBody(b, dt);
    for (auto& b : bodies)
        resolveBodyPlanes(b, dt);
}

void PhysicsWorld::resetBodies(const std::vector<RigidBody>& initial)
{
    bodies = initial;
}

void PhysicsWorld::setElasticity(float e)
{
    e = std::clamp(e, 0.0f, 1.0f);
    for (auto& b : bodies) b.restitution = e;
}

float PhysicsWorld::getElasticity() const
{
    return bodies.empty() ? 0.0f : bodies.front().restitution;
}

// ─── Integration ──────────────────────────────────────────────────────────

void PhysicsWorld::integrateBody(RigidBody& b, float dt) const
{
    // Linear integration with light air-drag to prevent infinite sliding
    b.velocity += kGravity * dt;
    b.velocity *= std::pow(0.999f, dt * 60.0f);
    b.position += b.velocity * dt;

    // Angular integration: integrate orientation from angular velocity
    float w = glm::length(b.angularVel);
    if (w > 1e-6f) {
        float angle = w * dt;
        glm::vec3 axis = b.angularVel / w;
        glm::quat dq = glm::angleAxis(angle, axis);
        b.orientation = glm::normalize(dq * b.orientation);
        // Light angular damping
        b.angularVel *= std::pow(0.995f, dt * 60.0f);
    }
}

// ─── Dispatch ─────────────────────────────────────────────────────────────

void PhysicsWorld::resolveBodyPlanes(RigidBody& b, float dt)
{
    for (const auto& p : planes) {
        // Bounds check for finite planes
        if (p.bounded) {
            if (b.position.x < p.boundsMin.x || b.position.x > p.boundsMax.x) continue;
            if (b.position.z < p.boundsMin.z || b.position.z > p.boundsMax.z) continue;
        }
        if (b.shape == ShapeType::Sphere) resolveSpherePlane(b, p, dt);
        else                              resolveBoxPlane   (b, p, dt);
    }
}

// ─── Sphere–plane ─────────────────────────────────────────────────────────
//
// Handles:
//  • elastic bounce    (|vn| > kMinBounce)
//  • inelastic contact (|vn| <= kMinBounce) — keeps ball on surface
//  • Coulomb friction  — both impulse-based (bounce) and gravity-based (rest)
//  • rolling torque    — angular velocity from friction at contact point

void PhysicsWorld::resolveSpherePlane(RigidBody& b, const CollisionPlane& p, float dt)
{
    // Signed gap: positive = sphere is clear of plane, negative = penetrating
    float dist = glm::dot(b.position - p.point, p.normal) - b.radius;
    if (dist >= 0.0f) return;

    // Push sphere out of the plane
    b.position -= p.normal * dist;  // dist < 0 → moves toward normal direction

    float vn = glm::dot(b.velocity, p.normal);

    // ── Normal impulse ──
    float jn = 0.0f;
    if (vn < 0.0f) {
        if (std::abs(vn) > kMinBounce) {
            // Elastic bounce
            float e = std::min(b.restitution, p.restitution);
            jn = -(1.0f + e) * vn;
        } else {
            // Micro-contact: zero the normal velocity without elastic bounce
            jn = -vn;
        }
        b.velocity += jn * p.normal;
    }

    // ── Friction ──
    // Tangential velocity (sliding direction)
    glm::vec3 vTang = b.velocity - glm::dot(b.velocity, p.normal) * p.normal;
    float vtLen = glm::length(vTang);
    if (vtLen < 1e-5f) return;

    float mu = std::sqrt(b.friction * p.friction);

    // Friction from the bounce impulse
    float frictionFromBounce = mu * std::abs(jn);

    // Gravity-based friction (handles resting contact on inclined surfaces):
    // F_n = m * |g · n|,  friction budget = mu * F_n * dt (converted to velocity)
    float fn = b.mass * std::abs(glm::dot(kGravity, p.normal));
    float frictionFromGravity = mu * fn * dt / b.mass;

    float totalFriction = std::min(frictionFromBounce + frictionFromGravity, vtLen);
    glm::vec3 tang = vTang / vtLen;
    b.velocity -= totalFriction * tang;

    // ── Rolling torque ──
    // r_contact = sphere center → contact point (opposite to normal direction)
    // I = 2/5 · m · r²  (solid sphere)
    glm::vec3 rContact = -b.radius * p.normal;
    float I = 0.4f * b.mass * b.radius * b.radius;
    // Friction impulse in momentum units = b.mass * totalFriction (opposing tang)
    b.angularVel += glm::cross(rContact, -totalFriction * tang * b.mass) / I;
}

// ─── Box–plane (impulse with full rotation response) ──────────────────────
//
// Finds the single deepest-penetrating corner and applies a rigid-body
// impulse at that contact point (linear + angular response).
// For multi-corner contact the error is small in practice.

void PhysicsWorld::resolveBoxPlane(RigidBody& b, const CollisionPlane& p, float dt)
{
    glm::mat3 R = glm::mat3_cast(b.orientation);
    const glm::vec3& h = b.halfExtents;

    // Enumerate all 8 corners in world space
    std::array<glm::vec3, 8> corners;
    int ci = 0;
    for (int sx : {-1, 1})
        for (int sy : {-1, 1})
            for (int sz : {-1, 1})
                corners[ci++] = b.position + R * glm::vec3(float(sx)*h.x, float(sy)*h.y, float(sz)*h.z);

    // Find the deepest penetrating corner
    float     minDist   = 0.0f;
    glm::vec3 contactPt = b.position;
    for (const auto& corner : corners) {
        float d = glm::dot(corner - p.point, p.normal);
        if (d < minDist) { minDist = d; contactPt = corner; }
    }
    if (minDist >= 0.0f) return;

    // Positional correction
    b.position -= p.normal * minDist;

    // Inertia tensor: I = m/3 · diag(hy²+hz², hx²+hz², hx²+hy²)
    glm::vec3 Idiag = (b.mass / 3.0f) * glm::vec3(
        h.y * h.y + h.z * h.z,
        h.x * h.x + h.z * h.z,
        h.x * h.x + h.y * h.y
    );

    // Apply inverse inertia in world space via body-frame diagonalisation
    auto applyInvI = [&](glm::vec3 v) -> glm::vec3 {
        glm::vec3 vb = glm::transpose(R) * v;
        vb /= Idiag;
        return R * vb;
    };

    // Velocity at the contact point: v_c = v + ω × r
    glm::vec3 r        = contactPt - b.position;
    glm::vec3 vContact = b.velocity + glm::cross(b.angularVel, r);
    float vn = glm::dot(vContact, p.normal);
    if (vn >= 0.0f) return; // already separating

    // Effective mass at contact in the normal direction
    glm::vec3 rCrossN = glm::cross(r, p.normal);
    float denom = 1.0f / b.mass + glm::dot(p.normal, glm::cross(applyInvI(rCrossN), r));

    float e  = std::min(b.restitution, p.restitution);
    float jn = (std::abs(vn) > kMinBounce)
               ? -(1.0f + e) * vn / denom
               :  -vn        / denom;

    b.velocity   += (jn / b.mass) * p.normal;
    b.angularVel += applyInvI(glm::cross(r, jn * p.normal));

    // ── Friction ──
    glm::vec3 vTang = vContact - vn * p.normal;
    float vtLen = glm::length(vTang);
    if (vtLen < 1e-5f) return;

    float mu = std::sqrt(b.friction * p.friction);
    // Gravity supplement for resting contact on inclines
    float fn = b.mass * std::abs(glm::dot(kGravity, p.normal));
    float gravFriction = mu * fn * dt / b.mass;

    float jf = std::min(mu * std::abs(jn) + gravFriction * denom, vtLen * denom);
    glm::vec3 tang = -vTang / vtLen;

    b.velocity   += (jf / b.mass) * tang;
    b.angularVel += applyInvI(glm::cross(r, jf * tang));
}

} // namespace physics
