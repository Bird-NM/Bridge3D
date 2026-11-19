#pragma once
#include <glm/glm.hpp>
struct Camera {
  glm::vec3 target{0, 28, 0}, desiredTarget{0, 28, 0};
  float yaw = .45f, pitch = .18f, distance = 270, desiredYaw = .45f,
        desiredPitch = .18f, desiredDistance = 270;
  bool ortho = false, smooth = true;
  glm::vec3 eye() const;
  glm::mat4 matrix(float aspect) const;
  void update(float dt);
  void preset(int view);
  void fit(glm::vec3 center, float radius, float aspect);
  void fitBounds(glm::vec3 minimum, glm::vec3 maximum, float aspect);
  void orbit(float dx, float dy);
  void pan(float dx, float dy, float height);
};
