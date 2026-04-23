#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace physics {

// ─── Collision plane (can be finite or infinite) ──────────────────────────

struct CollisionPlane {
    glm::vec3 point;
    glm::vec3 normal;        // unit outward normal
    float restitution = 0.6f;
    float friction    = 0.5f;

    // When bounded == true, collision is only tested if body position falls
    // inside the AABB [boundsMin, boundsMax] in world space.
    bool      bounded  = false;
    glm::vec3 boundsMin{};
    glm::vec3 boundsMax{};
};

// ─── Rigid body ───────────────────────────────────────────────────────────

enum class ShapeType { Sphere, Box };

struct RigidBody {
    ShapeType shape = ShapeType::Sphere;

    glm::vec3 position{};
    glm::vec3 velocity{};
    glm::quat orientation = glm::quat(1.f, 0.f, 0.f, 0.f); // identity
    glm::vec3 angularVel{};   // rad/s, world space

    float mass        = 1.0f;
    float restitution = 0.7f;
    float friction    = 0.4f;

    float     radius      = 0.5f;             // Sphere half-size
    glm::vec3 halfExtents = glm::vec3(0.4f);  // Box half-extents
};

// ─── Physics world ────────────────────────────────────────────────────────

class PhysicsWorld {
public:
    static constexpr float kGroundY = -1.5f;

    void addPlane(CollisionPlane p) { planes.push_back(std::move(p)); }
    void addBody (RigidBody     b) { bodies.push_back(std::move(b)); }

    void update(float dt);
    void resetBodies(const std::vector<RigidBody>& initial);

    // Convenience: set/get restitution on all bodies
    void  setElasticity(float e);
    float getElasticity() const;

    [[nodiscard]] const std::vector<RigidBody>&      getBodies() const { return bodies; }
    [[nodiscard]] const std::vector<CollisionPlane>& getPlanes() const { return planes; }

private:
    void integrateBody    (RigidBody& b, float dt) const;
    void resolveBodyPlanes(RigidBody& b, float dt);
    void resolveSpherePlane(RigidBody& b, const CollisionPlane& p, float dt);
    void resolveBoxPlane   (RigidBody& b, const CollisionPlane& p, float dt);

    std::vector<RigidBody>      bodies;
    std::vector<CollisionPlane> planes;

    static constexpr glm::vec3 kGravity   = {0.0f, -9.8f, 0.0f};
    static constexpr float     kMinBounce = 0.08f; // below this |vn|, use inelastic response
};

} // namespace physics
