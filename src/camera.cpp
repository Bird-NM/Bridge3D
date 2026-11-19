#include "camera.hpp"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
glm::vec3 Camera::eye() const {
  return target + distance * glm::vec3(std::sin(yaw) * std::cos(pitch),
                                       std::sin(pitch),
                                       std::cos(yaw) * std::cos(pitch));
}
glm::mat4 Camera::matrix(float aspect) const {
  float extent = distance * std::tan(glm::radians(23.f));
  auto p = ortho ? glm::ortho(-extent * aspect, extent * aspect, -extent,
                              extent, .1f, 3000.f)
                 : glm::perspective(glm::radians(46.f), aspect,
                                    std::max(.05f, distance / 2000),
                                    distance + 1500);
  return p * glm::lookAt(eye(), target, glm::vec3(0, 1, 0));
}
void Camera::update(float dt) {
  float k = smooth ? 1 - std::exp(-12 * dt) : 1;
  target = glm::mix(target, desiredTarget, k);
  yaw = glm::mix(yaw, desiredYaw, k);
  pitch = glm::mix(pitch, desiredPitch, k);
  distance = glm::mix(distance, desiredDistance, k);
}
void Camera::preset(int v) {
  desiredYaw = (v == 2 || v == 4) ? 1.570796f : 0.f;
  desiredPitch = v == 3 ? 1.55f : 0;
  if (v == 0) {
    desiredYaw = .45f;
    desiredPitch = .18f;
  }
}
void Camera::fit(glm::vec3 c, float r, float aspect) {
  desiredTarget = c;
  desiredDistance =
      glm::clamp(r /
                     std::sin(std::atan(std::tan(glm::radians(23.f)) *
                                        std::min(aspect, 1.f))) *
                     1.08f,
                 3.f, 2000.f);
}
void Camera::orbit(float dx, float dy) {
  desiredYaw -= dx * .006f;
  desiredPitch = glm::clamp(desiredPitch + dy * .006f, -1.5f, 1.55f);
}
void Camera::pan(float dx, float dy, float h) {
  auto right = glm::normalize(glm::cross(target - eye(), glm::vec3(0, 1, 0)));
  auto up = glm::normalize(glm::cross(right, target - eye()));
  desiredTarget += (-right * dx + up * dy) * distance * .85f / h;
}

void Camera::fitBounds(glm::vec3 minimum, glm::vec3 maximum, float aspect) {
  desiredTarget = (minimum + maximum) / 2.f;
  auto direction = glm::vec3(std::sin(desiredYaw) * std::cos(desiredPitch),
                             std::sin(desiredPitch),
                             std::cos(desiredYaw) * std::cos(desiredPitch));
  auto rotation =
      glm::mat3(glm::lookAt(direction, glm::vec3(0), glm::vec3(0, 1, 0)));
  float tangent = std::tan(glm::radians(23.f)), distance = 3;
  for (int x = 0; x < 2; ++x)
    for (int y = 0; y < 2; ++y)
      for (int z = 0; z < 2; ++z) {
        auto point = rotation * (glm::vec3(x ? maximum.x : minimum.x,
                                           y ? maximum.y : minimum.y,
                                           z ? maximum.z : minimum.z) -
                                 desiredTarget);
        float required = std::max(std::abs(point.x) / (tangent * aspect),
                                  std::abs(point.y) / tangent);
        distance = std::max(distance, required * 1.08f + (ortho ? 0 : point.z));
      }
  desiredDistance = glm::clamp(distance, 3.f, 2000.f);
}
