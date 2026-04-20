#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkApp {

/**
 * @brief Орбитальная камера с зумом / Orbital camera with zoom.
 *
 * Вращается вокруг начала координат по углам азимута и возвышения.
 * Orbits around the origin using azimuth and elevation angles.
 */
class Camera {
public:
    explicit Camera(float initialZ = kDefaultZ);

    // Управление / Controls
    void zoom(float delta);
    void orbit(float dAzimuth, float dElevation);
    void reset();

    // Матрица вида / View matrix
    [[nodiscard]] glm::mat4 viewMatrix() const;

    // Геттер / Accessor
    [[nodiscard]] float getZoomRadius() const noexcept { return zoomRadius; }

    // Константы / Constants
    static constexpr float kDefaultZ = 6.0f;
    static constexpr float kZMin = 2.0f;
    static constexpr float kZMax = 20.0f;
    static constexpr float kDefaultAz = 0.0f;
    static constexpr float kDefaultEl = 0.3f;  // ~17gr — slight elevation
    static constexpr float kElMax = 1.48f;     // ~85gr

private:
    float zoomRadius;     // Радиус / Radius
    float azimuthAngle;   // Угол в плоскости XZ / Azimuth in XZ plane (radians)
    float elevationAngle; // Угол над горизонтом / Elevation above horizon (radians)
};

} // namespace vkApp
