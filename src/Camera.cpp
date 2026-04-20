#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace vkApp {

Camera::Camera(float initialZ)
    : zoomRadius(initialZ)
    , azimuthAngle(kDefaultAz)
    , elevationAngle(kDefaultEl)
{}

void Camera::zoom(float delta) {
    zoomRadius = std::clamp(zoomRadius - delta, kZMin, kZMax);
}

void Camera::orbit(float dAzimuth, float dElevation) {
    azimuthAngle   += dAzimuth;
    elevationAngle  = std::clamp(elevationAngle + dElevation, -kElMax, kElMax);
}

void Camera::reset() {
    zoomRadius         = kDefaultZ;
    azimuthAngle   = kDefaultAz;
    elevationAngle = kDefaultEl;
}

glm::mat4 Camera::viewMatrix() const {
    // Сферические координаты / Spherical coordinates
    float cosEl = std::cos(elevationAngle);
    float sinEl = std::sin(elevationAngle);
    float cosAz = std::cos(azimuthAngle);
    float sinAz = std::sin(azimuthAngle);

    glm::vec3 eye(
        zoomRadius * cosEl * sinAz,
        zoomRadius * sinEl + 0.5f,   // небольшое смещение вверх / slight upward bias
        zoomRadius * cosEl * cosAz
    );

    return glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace vkApp
